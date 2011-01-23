/*
* Copyright (C) 2009 - 2010 TrinityCore <http://www.trinitycore.org/>
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
*/

#include "ScriptPCH.h"
#include "icecrown_citadel.h"

enum Yells
{
    SAY_AGGRO       = -1666063,
    SAY_BELOW_25    = -1666066,
    SAY_ABOVE_75    = -1666065,
    SAY_DEATH       = -1666067,
    SAY_PDEATH      = -1666068,
    SAY_END         = -1666070,
    SAY_BERSERK     = -1666069,
    SAY_OPEN_PORTAL = -1666064
};

enum Spells
{
    SPELL_CORRUPTION    = 70904,
    SPELL_DREAM_SLIP    = 71196,
    SPELL_RAGE          = 71189,
    SPELL_COLUMN        = 70704,
    SPELL_DREAM_STATE   = 70766,
    SPELL_VIGOR         = 70873,
    SPELL_TWISTED_NIGHTMARES = 71941,
    SPELL_CLOUD_VISUAL  = 70876,
    SPELL_PORTAL_N_PRE  = 71301,
    SPELL_PORTAL_N_NPC  = 71305,
    SPELL_PORTAL_H_PRE  = 71977,
    SPELL_PORTAL_H_NPC  = 71987,
    SPELL_SUMMON_PORTAL_TO_DREAM = 72482,
    SPELL_DREAM_PORTAL_VISUAL = 71304,
    SPELL_DREAM_PORTAL_VISUAL_2 = 70763
};

enum eEvents
{
    EVENT_SUMMON_DREAM_CLOUD        = 1,
    EVENT_DESPAWN                   = 3,
    //Events played when boss is healed up to 100%
    EVENT_BERSERK                   = 4,
    EVENT_EVADE_TO_DREAM_SLIP       = 5,

    //Adds
    EVENT_SUMMON_ADDS               = 8,
    EVENT_SUMMON_SUPRESSORS         = 9,
    EVENT_SUMMON_BLAZING_SKELETONS  = 10,
    EVENT_CAST_COLUMN_OF_FROST      = 11,
    EVENT_HASTEN_SUMMON_TIMER       = 12,

    //Portals
    EVENT_SUMMON_PORTALS_TO_DREAM   = 2,
    EVENT_OPEN_PORTAL_TO_DREAM      = 6,
    EVENT_CLOUD_EXPLODE             = 13,
    EVENT_CLOUD_EMULATE_DESPAWN     = 14,
    EVENT_CLOUD_EMULATE_RESPAWN     = 15,
    EVENT_INTRO                     = 7,
};
enum eValithriaData
{
    DATA_INSTANCE_DIFFICULTY = 1
};
//enum ePhases
//{
//    PHASE_NORMAL    = 0,
//    PHASE_WIN       = 1
//};
const Position Pos[] =
{
    {4239.579102f, 2566.753418f, 364.868439f, 0.0f}, //normal 0,1
    {4240.688477f, 2405.794678f, 364.868591f, 0.0f}, // normal
    {4165.112305f, 2405.872559f, 364.872925f, 0.0f}, //2,3
    {4166.216797f, 2564.197266f, 364.873047f, 0.0f}
};

void Cleanup(Creature *me, InstanceScript *instance)
{
    //Despawn heroic mode-specific mobs
    DespawnAllCreaturesAround(me, CREATURE_PORTAL_HEROIC_MODE_PRE);
    DespawnAllCreaturesAround(me, CREATURE_PORTAL_HEROIC_MODE_NPC);
    DespawnAllCreaturesAround(me, CREATURE_NIGHTMARE_CLOUD);
    
    //Despawn normal mode-specific mobs
    DespawnAllCreaturesAround(me, CREATURE_PORTAL_NORMAL_MODE_PRE);
    DespawnAllCreaturesAround(me, CREATURE_PORTAL_NORMAL_MODE_NPC);
    DespawnAllCreaturesAround(me, CREATURE_DREAM_CLOUD);

    DespawnAllCreaturesAround(me, CREATURE_ZOMBIE);
    DespawnAllCreaturesAround(me, CREATURE_SKELETON);
    DespawnAllCreaturesAround(me, CREATURE_ARCHMAGE);
    DespawnAllCreaturesAround(me, CREATURE_SUPPRESSER);
    DespawnAllCreaturesAround(me, CREATURE_ABOMINATION);
    DespawnAllCreaturesAround(me, CREATURE_ROT_WORM);
    DespawnAllCreaturesAround(me, CREATURE_COLUMN_OF_FROST);
    DespawnAllCreaturesAround(me, CREATURE_MANA_VOID);

    instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_VIGOR);
    instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_TWISTED_NIGHTMARES);
    instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_DREAM_STATE);
}


class boss_valithria : public CreatureScript
{
    public:
        boss_valithria() : CreatureScript("boss_valithria") { }

        struct boss_valithriaAI : public BossAI
        {
            boss_valithriaAI(Creature* pCreature) : BossAI(pCreature, DATA_VALITHRIA_DREAMWALKER)
            {
            }

            void Reset()
            {
                events.Reset();

                DoCast(me, SPELL_CORRUPTION);
                me->SetHealth(uint32(me->GetMaxHealth() / 2));

                bIntro = false;
                bEnd = false;
                bAboveHP = false;
                bBelowHP = false;

                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);

                instance->SetData(DATA_VALITHRIA_DREAMWALKER_EVENT, NOT_STARTED);
            }

            void MoveInLineOfSight(Unit *who)
            {
                if (instance->GetData(DATA_VALITHRIA_DREAMWALKER_EVENT) == NOT_STARTED
                    && who->IsWithinDistInMap(me, 40.0f,true))
                {
                    //instance->SetData(DATA_VALITHRIA_DREAMWALKER_EVENT, IN_PROGRESS);
                    if (Creature* combatTrigger = me->GetCreature(*me, instance->GetData64(DATA_VALITHRIA_COMBAT_TRIGGER)))
                        ((ScriptedAI*)combatTrigger->GetAI())->AttackStartNoMove(who);
                    events.ScheduleEvent(EVENT_INTRO, 10000);
                }
  
                //if (instance->GetData(DATA_VALITHRIA_DREAMWALKER_EVENT) == IN_PROGRESS)
                //    if(!who->IsWithinDistInMap(me, 60.0f,true))
                //    {
                //        instance->SetData(DATA_VALITHRIA_DREAMWALKER_EVENT, FAIL);
                //        events.Reset();
                //    }
            }

            void JustReachedHome()
            {
            }

            void JustDied(Unit* /*pKiller*/)
            {
            }
            uint32 GetData(uint32 id)
            {
                if (id == DATA_INSTANCE_DIFFICULTY)
                    return RAID_MODE(10, 25, 10, 25);
                return BossAI::GetData(id);
            }
            void CheckHealth()
            {
                if (instance->GetData(DATA_VALITHRIA_DREAMWALKER_EVENT) == DONE)
                    return;
                if (HealthBelowPct(2))
                {
                    events.Reset();
                    DoScriptText(SAY_DEATH, me);
                    instance->SetData(DATA_VALITHRIA_DREAMWALKER_EVENT, FAIL);
                    if (Creature* combatTrigger = me->GetCreature(*me, instance->GetData64(DATA_VALITHRIA_COMBAT_TRIGGER)))
                        combatTrigger->AI()->EnterEvadeMode();
                    return;
                }
                if (!bAboveHP && HealthAbovePct(74))
                {
                    DoScriptText(SAY_ABOVE_75, me);
                    //Need to increase her in size
                    bAboveHP = true;
                    bBelowHP = false;
                }
                if (!bBelowHP && HealthBelowPct(26))
                {
                    DoScriptText(SAY_BELOW_25, me);
                    //Need to decrease her in size
                    bBelowHP = true;
                    bAboveHP = false;
                }
                if (!bEnd && HealthAbovePct(99))
                {
                    DoScriptText(SAY_END, me);
                    me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
                    me->SetReactState(REACT_PASSIVE);
                    me->RemoveAurasDueToSpell(SPELL_CORRUPTION);
                    bEnd = true;
                    events.Reset();
                    events.ScheduleEvent(EVENT_BERSERK, 1000);
                }
            }

            void DamageTaken(Unit* /*done_by*/, uint32& /*damage*/)
            {
                CheckHealth();
            }

            void HealReceived(Unit* /*done_by*/, uint32& /*addhealth*/)
            {
                CheckHealth();
            }

            void UpdateAI(const uint32 diff)
            {
                events.Update(diff);

                if (me->HasUnitState(UNIT_STAT_CASTING))
                    return;

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_INTRO:
                        {
                            DoScriptText(SAY_AGGRO, me);
                            events.ScheduleEvent(EVENT_SUMMON_ADDS, 10000);
                            //For debug purposes it's scheduled earlier, 5 seconds instead of 35
                            events.ScheduleEvent(EVENT_SUMMON_PORTALS_TO_DREAM, 5000);
                            break;
                        }
                        case EVENT_SUMMON_PORTALS_TO_DREAM:
                        {
                            DoScriptText(SAY_OPEN_PORTAL, me);
                            //DoCast(SPELL_SUMMON_PORTAL_TO_DREAM);
                            //Need to check, maybe, this isn't required
                            for(uint8 p = 0; p < RAID_MODE<uint8>(3, 8, 3, 8); ++p)
                                DoCast(RAID_MODE(SPELL_PORTAL_N_PRE,SPELL_PORTAL_N_PRE,SPELL_PORTAL_H_PRE,SPELL_PORTAL_H_PRE));
                            if (Creature* valithriaAlternative = Unit::GetCreature(*me, instance->GetData64(DATA_VALITHRIA_ALTERNATIVE)))
                                valithriaAlternative->AI()->DoAction(EVENT_SUMMON_DREAM_CLOUD);
                            events.ScheduleEvent(EVENT_SUMMON_PORTALS_TO_DREAM, 50000);
                            break;
                        }
                        case EVENT_BERSERK:
                        {
                            //DoScriptText(SAY_BERSERK, me);
                            DoCast(me, SPELL_RAGE);
                            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                            events.ScheduleEvent(EVENT_EVADE_TO_DREAM_SLIP, 6000);
                            break;
                        }
                        case EVENT_EVADE_TO_DREAM_SLIP:
                        {
                            DoCast(SPELL_DREAM_SLIP);
                            events.ScheduleEvent(EVENT_DESPAWN, 1000);
                            break;
                        }
                        case EVENT_DESPAWN:
                        {
                            Cleanup(me, instance);
                            instance->SetData(DATA_VALITHRIA_DREAMWALKER_EVENT, DONE);
                            if (Creature* combatTrigger = me->GetCreature(*me, instance->GetData64(DATA_VALITHRIA_COMBAT_TRIGGER)))
                                combatTrigger->DespawnOrUnsummon();
                            me->DespawnOrUnsummon();
                            break;
                        }
                        default:
                            break;
                    }
                }
            }

        private:

            bool bIntro;
            bool bEnd;
            bool bAboveHP;
            bool bBelowHP;
        };

        CreatureAI* GetAI(Creature* pCreature) const
        {
            return new boss_valithriaAI(pCreature);
        }
};

//Dream related NCPs and spells
class npc_valithria_alternative : public CreatureScript
{
    public:
        npc_valithria_alternative() : CreatureScript("npc_valithria_alternative") { }

        struct npc_valithria_alternativeAI : public ScriptedAI
        {
            npc_valithria_alternativeAI(Creature* pCreature) : ScriptedAI(pCreature)
            {
                pInstance = pCreature->GetInstanceScript();
                numberOfClouds = RAID_MODE<uint8>(15, 50, 50, 100);
            }

            void Reset()
            {
                events.Reset();
                me->SetPhaseMask(16, true);
                me->AddUnitMovementFlag(MOVEMENTFLAG_CAN_FLY);
                me->SendMovementFlagUpdate();
                me->SetFlying(true);
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                //events.ScheduleEvent(EVENT_SUMMON_DREAM_CLOUD, 15000);
                me->GetPosition(x,y,z);
                bAlreadySummonedClouds = false;
            }
            void DoAction(const int32 action)
            {
                //Clouds do not respawn in heroic mode
                if (action == EVENT_SUMMON_DREAM_CLOUD && !(IsHeroic() && bAlreadySummonedClouds))
                {
                    bAlreadySummonedClouds = true;
                    const uint32 cloudEntry = IsHeroic() ? CREATURE_NIGHTMARE_CLOUD : CREATURE_DREAM_CLOUD;
                    DespawnAllCreaturesAround(me, cloudEntry);
                    float angle, summonX, summonY, summonZ, distMin = 20.0f, distMax = 35.0f, zVariation = 15.0f, summonDist;
                    //Summon Dream clouds in a circle pattern, but randomize their positions a little (random angle and random Z coordinate)
                    for(uint8 i = 0; i <= numberOfClouds; ++i) //need correct count
                    {
                        //angle = (float)rand_norm() * static_cast<float>(2 * M_PI);
                        //Distribute portals evenly around the boss, with a little angle variation
                        angle = static_cast<float>(2 * M_PI) * i / numberOfClouds + (float)rand_norm() * 0.07;
                        summonDist = distMin + (distMax - distMin) * (float)rand_norm();
                        summonX = x + summonDist * cos(angle);
                        summonY = y + summonDist * sin(angle);
                        summonZ = z + (float)rand_norm() * zVariation - zVariation / 2.0f;
                        //Trinity::NormalizeMapCoord(summonX);
                        //Trinity::NormalizeMapCoord(summonY);
                        me->SummonCreature(cloudEntry, summonX, summonY, summonZ, 0, TEMPSUMMON_TIMED_DESPAWN, IsHeroic() ? 600000 : 50000);
                    }
                }
            }
            //void UpdateAI(const uint32 diff)
            //{
            //    if (pInstance && pInstance->GetData(DATA_VALITHRIA_DREAMWALKER_EVENT) != IN_PROGRESS)
            //        return;
            //    events.Update(diff);

            //    while (uint32 eventId = events.ExecuteEvent())
            //    {
            //        switch (eventId)
            //        {
            //            case EVENT_SUMMON_DREAM_CLOUD:
            //            {

            //            }
            //            default:
            //                break;
            //        }
            //    }
            //}

          private:
            InstanceScript* pInstance;
            bool bAlreadySummonedClouds;
            float x, y, z;
            uint8 numberOfClouds;
            EventMap events;
        };

        CreatureAI* GetAI(Creature* pCreature) const
        {
            return new npc_valithria_alternativeAI(pCreature);
        }
};

class npc_dreamportal_icc : public CreatureScript //������ � �������
{
    public:
        npc_dreamportal_icc() : CreatureScript("npc_dreamportal_icc") { }

        struct npc_dreamportal_iccAI : public Scripted_NoMovementAI
        {
            npc_dreamportal_iccAI(Creature* pCreature) : Scripted_NoMovementAI(pCreature) { }

            void Reset()
            {
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_STUNNED);
                me->SetSpeed(MOVE_WALK, 0.0f);
                me->SetSpeed(MOVE_RUN, 0.0f);
                me->SetReactState(REACT_PASSIVE);
                events.Reset();
                events.ScheduleEvent(EVENT_OPEN_PORTAL_TO_DREAM, 15000);
                //DoStartNoMovement();
            }
            void DoAction(const int32 action)
            {
                if (action == EVENT_SPELLCLICK)
                    events.ScheduleEvent(EVENT_DESPAWN, 100);
            }
            void UpdateAI(const uint32 diff)
            {
                events.Update(diff);
                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_OPEN_PORTAL_TO_DREAM:
                        {
                            if(me->GetEntry() == CREATURE_PORTAL_NORMAL_MODE_PRE)
                            {
                                me->UpdateEntry(CREATURE_PORTAL_NORMAL_MODE_NPC);
                                if (urand(0, 1))
                                    DoCast(me, SPELL_DREAM_PORTAL_VISUAL);
                                else
                                    DoCast(me, SPELL_DREAM_PORTAL_VISUAL_2);
                            }
                            else
                                if(me->GetEntry() == CREATURE_PORTAL_HEROIC_MODE_PRE)
                                {
                                    me->UpdateEntry(CREATURE_PORTAL_HEROIC_MODE_NPC);
                                    if (urand(0, 1))
                                        DoCast(me, SPELL_DREAM_PORTAL_VISUAL);
                                    else
                                        DoCast(me, SPELL_DREAM_PORTAL_VISUAL_2);
                                }
                            events.ScheduleEvent(EVENT_DESPAWN, 15000);
                            break;
                        }
                        case EVENT_DESPAWN:
                        {
                            me->DespawnOrUnsummon();
                            break;
                        }
                        default:
                            break;
                    }
                }
                

            }

        private:
            EventMap events;
            uint32 m_uiChangeTimer;
        };

        CreatureAI* GetAI(Creature* pCreature) const
        {
            return new npc_dreamportal_iccAI(pCreature);
        }
};

class npc_dreamcloud_icc : public CreatureScript 
{
    public:
        npc_dreamcloud_icc() : CreatureScript("npc_dreamcloud_icc") { }

        struct npc_dreamcloud_iccAI : public ScriptedAI
        {
            npc_dreamcloud_iccAI(Creature* pCreature) : ScriptedAI(pCreature) { }

            void Reset()
            {
                canApplyBuff = true;
                events.Reset();
                me->SetPhaseMask(16, true);
                DoCast(SPELL_CLOUD_VISUAL);
                me->AddUnitMovementFlag(MOVEMENTFLAG_CAN_FLY);
                me->SendMovementFlagUpdate();
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                me->SetReactState(REACT_AGGRESSIVE);
            }

            void MoveInLineOfSight(Unit *who)
            {
                if (canApplyBuff && me->IsWithinDistInMap(who, 5.0f, true) && who->GetTypeId() == TYPEID_PLAYER)
                {
                    canApplyBuff = false;
                    //uiPlayerTriggered = who->GetGUID();
                    //Cloud will explode in about 1-2 seconds after player touch it
                    events.ScheduleEvent(EVENT_CLOUD_EXPLODE, 1000);
                }
                ScriptedAI::MoveInLineOfSight(who);
            }

            void UpdateAI(const uint32 diff)
            {
                events.Update(diff);

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_CLOUD_EXPLODE:
                        {
                            //Apply buff to player
                            const uint32 spellId = IsHeroic() ? SPELL_TWISTED_NIGHTMARES : SPELL_VIGOR;
                            //Apply buff only if player is within 5 feet of the cloud
                            DoCast(me, spellId, true);

                            //Just show explode animation effect
                            events.ScheduleEvent(EVENT_CLOUD_EMULATE_DESPAWN, 2000);
                            break;
                        }
                        case EVENT_CLOUD_EMULATE_DESPAWN:
                        {
                            me->SetVisible(false);
                            //In normal mode, the clouds have a very fast respawn so you won�t run the risk of running out of them. 
                            if (!IsHeroic())
                                events.ScheduleEvent(EVENT_CLOUD_EMULATE_RESPAWN, 7000);
                            break;
                        }
                        case EVENT_CLOUD_EMULATE_RESPAWN:
                        {
                            me->SetVisible(true);
                            canApplyBuff = true;
                        }
                        default:
                            break;
                    }
                }
            }
        private:
            EventMap events;
            bool canApplyBuff;
            uint64 uiPlayerTriggered;
        };

        CreatureAI* GetAI(Creature* pCreature) const
        {
            return new npc_dreamcloud_iccAI(pCreature);
        }
};

class spell_valithria_summon_portal : public SpellScriptLoader
{
    public:
        spell_valithria_summon_portal() : SpellScriptLoader("spell_valithria_summon_portal") { }

        class spell_valithria_summon_portal_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_valithria_summon_portal_SpellScript);

            void ChangeSummonPos(SpellEffIndex /*effIndex*/)
            {
                if (Creature *valithria = GetCaster()->ToCreature())
                {
                    uint32 instanceDifficulty = valithria->GetAI()->GetData(DATA_INSTANCE_DIFFICULTY);
                    float x, y, distMin = 15.0f, distMax = 30.0f;
                    valithria->GetPosition(x, y);
                    float maxAngle = ((instanceDifficulty == 25) ?  static_cast<float>(2 * M_PI) :  static_cast<float>(M_PI));
                    //Decrease ange by 90 degreese because portals should be spawned in front of boss in 10-man modes
                    float angle = (float)rand_norm() * maxAngle - static_cast<float>(M_PI) / 2.0f;
                    float summonDist = distMin + (distMax - distMin) * (float)rand_norm();
                    float summonX = x + summonDist * cos(angle);
                    float summonY = y + summonDist * sin(angle);

                    WorldLocation* summonPos = GetTargetDest();
                    summonPos->Relocate(summonX, summonY); 
                }
            }

            void Register()
            {
                OnEffect += SpellEffectFn(spell_valithria_summon_portal_SpellScript::ChangeSummonPos, EFFECT_0, SPELL_EFFECT_SUMMON);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_valithria_summon_portal_SpellScript();
        }
};

class spell_valithria_vigor : public SpellScriptLoader
{
    public:
        spell_valithria_vigor() : SpellScriptLoader("spell_valithria_vigor") { }

        class spell_valithria_vigor_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_valithria_vigor_SpellScript);

            bool Load()
            {
                prevented = false;
                return true;
            }

            //void HandleScript(SpellEffIndex effIndex)
            void EnsureAuraStack()
            {
                if (!(GetHitUnit() && GetHitUnit()->isAlive()))
                    return;
                if (GetHitUnit()->GetGUID() == GetCaster()->GetGUID())
                    return;

                if (Unit *who = GetHitUnit())
                {
                    uint32 curSpellId = GetSpellInfo()->Id;
                    if (Aura* auraVigor = who->GetAura(curSpellId))
                    {
                        prevented = true;
                        prevStackAmount = auraVigor->GetStackAmount();
                    }
                }
            }

            void RemoveImmunity()
            {
                if (Unit* target = GetHitUnit())
                {
                    if (prevented)
                    {
                        if (Aura* auraVigor = GetHitUnit()->GetAura(GetSpellInfo()->Id))
                        {
                            auraVigor->SetStackAmount(prevStackAmount+1);
                            auraVigor->RefreshDuration();
                        }
                    }
                }
            }

            void Register()
            {
                BeforeHit += SpellHitFn(spell_valithria_vigor_SpellScript::EnsureAuraStack);
                AfterHit += SpellHitFn(spell_valithria_vigor_SpellScript::RemoveImmunity);
            }
        private:
            bool prevented;
            uint8 prevStackAmount;
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_valithria_vigor_SpellScript();
        }
};

//End of Dream related NPCs and spells
class npc_icc_combat_stalker : public CreatureScript
{
    public:
        npc_icc_combat_stalker() : CreatureScript("npc_icc_combat_stalker") { }

        struct npc_icc_combat_stalkerAI : public ScriptedAI
        {
            npc_icc_combat_stalkerAI(Creature* pCreature) : ScriptedAI(pCreature), summons(me)
            {
                pInstance = me->GetInstanceScript();
            }

            void InitializeAI()
            {
                //if (!me->isDead())
                //   Reset();
                EnterEvadeIfOutOfCombatArea(60.0f);
            }
            void Reset()
            {
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                //me->SetInCombatWithZone();
                DoStartNoMovement(me->getVictim());
                events.Reset();
                //For debug purposes
                //events.ScheduleEvent(EVENT_SUMMON_ADDS, 20000);
                //events.ScheduleEvent(EVENT_SUMMON_SUPRESSORS, 20000);
                //events.ScheduleEvent(EVENT_SUMMON_BLAZING_SKELETONS, 20000);
                //events.ScheduleEvent(EVENT_CAST_COLUMN_OF_FROST, 13000);
                //events.ScheduleEvent(EVENT_HASTEN_SUMMON_TIMER, 30000);
                m_uiSummonTimer = 2000;
                if (pInstance && pInstance->GetData(DATA_VALITHRIA_DREAMWALKER_EVENT) != IN_PROGRESS)
                    summons.DespawnAll();
                if (Creature* valithria = Unit::GetCreature(*me, pInstance->GetData64(DATA_VALITHRIA_DREAMWALKER)))
                    valithria->AI()->Reset();
            }

            void EnterCombat(Unit* /*who*/)
            {
                DoZoneInCombat(me);
                pInstance->SetData(DATA_VALITHRIA_DREAMWALKER_EVENT, IN_PROGRESS);
            }

            void JustReachedHome()
            {
                pInstance->SetData(DATA_VALITHRIA_DREAMWALKER_EVENT, FAIL);
                if (Creature* valithria = Unit::GetCreature(*me, pInstance->GetData64(DATA_VALITHRIA_DREAMWALKER)))
                    valithria->AI()->EnterEvadeMode();
            }

            void EnterEvadeMode()
            {
                if (Creature* valithria = Unit::GetCreature(*me, pInstance->GetData64(DATA_VALITHRIA_DREAMWALKER)))
                    valithria->AI()->EnterEvadeMode();
                pInstance->DoCastSpellOnPlayers(UPPER_SPIRE_TELEPORT);
            }

            void JustSummoned(Creature* pSummoned)
            {
                if (Creature* valithria = Unit::GetCreature(*me, pInstance->GetData64(DATA_VALITHRIA_DREAMWALKER)))
                    pSummoned->AI()->AttackStart(valithria);

                summons.Summon(pSummoned);
            }

            void KilledUnit(Unit* /*victim*/)
            {
                if (Creature* valithria = Unit::GetCreature(*me, pInstance->GetData64(DATA_VALITHRIA_DREAMWALKER)))
                    DoScriptText(SAY_PDEATH, valithria);
            }

            void UpdateAI(const uint32 diff)
            {
                if (!pInstance || pInstance->GetData(DATA_VALITHRIA_DREAMWALKER_EVENT) != IN_PROGRESS)
                    return;
                events.Update(diff);
                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_SUMMON_ADDS:
                        {
                            for (uint8 coords = 0; coords <= RAID_MODE(1,3,1,3); ++coords)
                            {
                                if(urand(0,1)) DoSummon(CREATURE_ZOMBIE, Pos[coords]);
                                if(urand(0,1)) DoSummon(CREATURE_ARCHMAGE, Pos[coords]);
                                if(urand(0,1)) DoSummon(CREATURE_ABOMINATION, Pos[coords]);
                            }
                            break;
                        }
                        case EVENT_SUMMON_SUPRESSORS:
                        {
                            for (uint8 coords = 0; coords <= RAID_MODE(1,3,1,3); ++coords)
                                if(urand(0,1)) DoSummon(CREATURE_SUPPRESSER, Pos[coords]);
                            events.ScheduleEvent(EVENT_SUMMON_SUPRESSORS, urand(m_uiSummonTimer-2000, m_uiSummonTimer+2000));
                            break;
                        }
                        case EVENT_SUMMON_BLAZING_SKELETONS:
                        {
                            for (uint8 coords = 0; coords <= RAID_MODE(1,3,1,3); ++coords)
                                if(urand(0,1)) DoSummon(CREATURE_SKELETON, Pos[coords]);
                            events.ScheduleEvent(EVENT_SUMMON_BLAZING_SKELETONS, urand(m_uiSummonTimer-2000, m_uiSummonTimer+2000));
                            break;
                        }
                        case EVENT_HASTEN_SUMMON_TIMER:
                        {
                            m_uiSummonTimer -= 1000;
                            if (m_uiSummonTimer < 2000)
                                m_uiSummonTimer = 2000;
                            events.ScheduleEvent(EVENT_HASTEN_SUMMON_TIMER, 30000);
                            break;
                        }
                        case EVENT_CAST_COLUMN_OF_FROST:
                        {
                            if(Unit* pTarget = SelectTarget(SELECT_TARGET_RANDOM, 1, 100.0f, true))
                                DoCast(pTarget, SPELL_COLUMN);
                            events.ScheduleEvent(EVENT_CAST_COLUMN_OF_FROST, 13000);
                            break;
                        }
                        default: 
                            break;
                    }

                }

            }
            private:
                InstanceScript* pInstance;

                uint32 m_uiSummonTimer;
                SummonList summons;
                EventMap events;
        };

        CreatureAI* GetAI(Creature* pCreature) const
        {
            return new npc_icc_combat_stalkerAI(pCreature);
        }
};

void AddSC_boss_valithria()
{
    new boss_valithria();
    new npc_dreamportal_icc();
    new npc_dreamcloud_icc();
    new npc_icc_combat_stalker();
    new npc_valithria_alternative();
    new spell_valithria_summon_portal();
    new spell_valithria_vigor();
}