//========= Copyright � 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose:		Antlion Grub - cannon fodder
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "AI_Task.h"
#include "AI_Default.h"
#include "AI_Schedule.h"
#include "AI_Hull.h"
#include "AI_Navigator.h"
#include "activitylist.h"
#include "game.h"
#include "NPCEvent.h"
#include "Player.h"
#include "EntityList.h"
#include "AI_Interactions.h"
#include "soundent.h"
#include "Gib.h"
#include "soundenvelope.h"
#include "shake.h"
#include "Sprite.h"
#include "vstdlib/random.h"
#include "npc_antliongrub_beta.h"
#include "engine/IEngineSound.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar	sk_antliongrub_health( "sk_antliongrub_health", "0" );

#define	ANTLIONGRUB_SQUEAL_DIST			512
#define	ANTLIONGRUB_SQUASH_TIME			1.0f
#define	ANTLIONGRUB_HEAL_RANGE			256.0f
#define	ANTLIONGRUB_HEAL_CONE			0.5f
#define	ANTLIONGRUB_ENEMY_HOSTILE_TIME	8.0f

//Sharp fast attack
envelopePoint_t envFastAttack[] =
{
	{	0.3f, 0.5f,	//Attack
		0.5f, 1.0f,
	},
	{	0.0f, 0.1f, //Decay
		0.1f, 0.2f,
	},
	{	-1.0f, -1.0f, //Sustain
		1.0f, 2.0f,
	},
	{	0.0f, 0.0f,	//Release
		0.5f, 1.0f,
	}
};

//Slow start to fast end attack
envelopePoint_t envEndAttack[] =
{
	{	0.0f, 0.1f,
		0.5f, 1.0f,
	},
	{	-1.0f, -1.0f,
		0.5f, 1.0f,
	},
	{	0.3f, 0.5f,
		0.2f, 0.5f,
	},
	{	0.0f, 0.0f,
		0.5f, 1.0f,
	},
};

//rise, long sustain, release
envelopePoint_t envMidSustain[] =
{
	{	0.2f, 0.4f,
		0.1f, 0.5f,
	},
	{	-1.0f, -1.0f,
		0.1f, 0.5f,
	},
	{	0.0f, 0.0f,
		0.5f, 1.0f,
	},
};

//Scared
envelopePoint_t envScared[] =
{
	{	0.8f, 1.0f,
		0.1f, 0.2f,
	},
	{
		-1.0f, -1.0f,
		0.25f, 0.5f
	},
	{	0.0f, 0.0f,
		0.5f, 1.0f,
	},
};

//Grub voice envelopes
envelopeDescription_t grubVoiceEnvelopes[] = 
{
	{ envFastAttack,	ARRAYSIZE(envFastAttack) },
	{ envEndAttack,		ARRAYSIZE(envEndAttack) },
	{ envMidSustain,	ARRAYSIZE(envMidSustain) },
};

//Data description
BEGIN_DATADESC( CNPC_AntlionGrub )

	DEFINE_SOUNDPATCH( m_pMovementSound ),
	DEFINE_SOUNDPATCH( m_pVoiceSound ),
	DEFINE_SOUNDPATCH( m_pHealSound ),
	DEFINE_FIELD( m_pGlowSprite,			FIELD_CLASSPTR ),
	DEFINE_FIELD( m_flNextVoiceChange,	FIELD_TIME ),
	DEFINE_FIELD( m_flSquashTime,			FIELD_TIME ),
	DEFINE_FIELD( m_flNearTime,			FIELD_TIME ),
	DEFINE_FIELD( m_flHealthTime,			FIELD_TIME ),
	DEFINE_FIELD( m_flEnemyHostileTime,	FIELD_TIME ),
	DEFINE_FIELD( m_bSquashed,			FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bMoving,				FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bSquashValid,			FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bHealing,				FIELD_BOOLEAN ),
	DEFINE_FIELD( m_nHealthReserve,		FIELD_INTEGER ),
	DEFINE_FIELD( m_nGlowSpriteHandle,	FIELD_INTEGER ),

	// Functions
	DEFINE_ENTITYFUNC( GrubTouch ),

END_DATADESC()

//Schedules
enum AntlionGrubSchedules
{	 
	SCHED_ANTLIONGRUB_SQUEAL = LAST_SHARED_SCHEDULE,
	SCHED_ANTLIONGRUB_SQUIRM,
	SCHED_ANTLIONGRUB_STAND,
	SCHED_ANTLIONGRUB_GIVE_HEALTH,
	SCHED_ANTLIONGUARD_RETREAT,
};

//Tasks
enum AntlionGrubTasks
{	
	TASK_ANTLIONGRUB_SQUIRM = LAST_SHARED_TASK,
	TASK_ANTLIONGRUB_GIVE_HEALTH,
	TASK_ANTLIONGRUB_MOVE_TO_TARGET,
	TASK_ANTLIONGRUB_FIND_RETREAT_GOAL,
};

//Conditions
enum AntlionGrubConditions
{
	COND_ANTLIONGRUB_HEARD_SQUEAL = LAST_SHARED_CONDITION,
	COND_ANTLIONGRUB_BEING_SQUASHED,
	COND_ANTLIONGRUB_IN_HEAL_RANGE,
};

//Activities
int	ACT_ANTLIONGRUB_SQUIRM;
int ACT_ANTLIONGRUB_HEAL;

//Animation events
#define ANTLIONGRUB_AE_START_SQUIRM	11	//Start squirming portion of animation
#define ANTLIONGRUB_AE_END_SQUIRM	12	//End squirming portion of animation

//Interaction IDs
int	g_interactionAntlionGrubAlert = 0;

#define	REGISTER_INTERACTION( a )	{ a = CBaseCombatCharacter::GetInteractionID(); }

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CNPC_AntlionGrub::CNPC_AntlionGrub( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_AntlionGrub::InitCustomSchedules( void ) 
{
	INIT_CUSTOM_AI( CNPC_AntlionGrub );

	//Schedules
	ADD_CUSTOM_SCHEDULE( CNPC_AntlionGrub,	SCHED_ANTLIONGRUB_SQUEAL );
	ADD_CUSTOM_SCHEDULE( CNPC_AntlionGrub,	SCHED_ANTLIONGRUB_SQUIRM );
	ADD_CUSTOM_SCHEDULE( CNPC_AntlionGrub,	SCHED_ANTLIONGRUB_STAND );
	ADD_CUSTOM_SCHEDULE( CNPC_AntlionGrub,	SCHED_ANTLIONGRUB_GIVE_HEALTH );
	ADD_CUSTOM_SCHEDULE( CNPC_AntlionGrub,	SCHED_ANTLIONGUARD_RETREAT );
	
	//Tasks
	ADD_CUSTOM_TASK( CNPC_AntlionGrub,		TASK_ANTLIONGRUB_SQUIRM );
	ADD_CUSTOM_TASK( CNPC_AntlionGrub,		TASK_ANTLIONGRUB_GIVE_HEALTH );
	ADD_CUSTOM_TASK( CNPC_AntlionGrub,		TASK_ANTLIONGRUB_MOVE_TO_TARGET );
	ADD_CUSTOM_TASK( CNPC_AntlionGrub,		TASK_ANTLIONGRUB_FIND_RETREAT_GOAL );

	//Conditions
	ADD_CUSTOM_CONDITION( CNPC_AntlionGrub,	COND_ANTLIONGRUB_HEARD_SQUEAL );
	ADD_CUSTOM_CONDITION( CNPC_AntlionGrub,	COND_ANTLIONGRUB_BEING_SQUASHED );
	ADD_CUSTOM_CONDITION( CNPC_AntlionGrub,	COND_ANTLIONGRUB_IN_HEAL_RANGE );

	//Activities
	ADD_CUSTOM_ACTIVITY( CNPC_AntlionGrub,	ACT_ANTLIONGRUB_SQUIRM );
	ADD_CUSTOM_ACTIVITY( CNPC_AntlionGrub,	ACT_ANTLIONGRUB_HEAL );

	AI_LOAD_SCHEDULE( CNPC_AntlionGrub,	SCHED_ANTLIONGRUB_SQUEAL );
	AI_LOAD_SCHEDULE( CNPC_AntlionGrub,	SCHED_ANTLIONGRUB_SQUIRM );
	AI_LOAD_SCHEDULE( CNPC_AntlionGrub,	SCHED_ANTLIONGRUB_STAND );
	AI_LOAD_SCHEDULE( CNPC_AntlionGrub,	SCHED_ANTLIONGRUB_GIVE_HEALTH );
	AI_LOAD_SCHEDULE( CNPC_AntlionGrub,	SCHED_ANTLIONGUARD_RETREAT );
}

LINK_ENTITY_TO_CLASS( npc_antliongrub, CNPC_AntlionGrub );
IMPLEMENT_CUSTOM_AI( npc_antliongrub, CNPC_AntlionGrub );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_AntlionGrub::Precache( void )
{
	PrecacheModel( ANTLIONGRUB_MODEL );
	PrecacheModel( ANTLIONGRUB_SQUASHED_MODEL );

	m_nGlowSpriteHandle = PrecacheModel("sprites/blueflare1.vmt");

	PrecacheScriptSound( "NPC_AntlionGrub.Scared" );
	PrecacheScriptSound( "NPC_AntlionGrub.Squash" );

	PrecacheScriptSound( "NPC_Antlion.Movement" );
	PrecacheScriptSound( "NPC_Antlion.Voice" );
	PrecacheScriptSound( "NPC_Antlion.Heal" );

	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_AntlionGrub::Spawn( void )
{
	Precache();

	SetModel( ANTLIONGRUB_MODEL );

	SetHullType(HULL_TINY);
	
	Vector	vecMins, vecMaxs;

	vecMins = GetHullMins();
	vecMaxs = GetHullMaxs();

	//Just the perfect size for being stepped on
	vecMaxs[2] -= 14.0f;

	UTIL_SetSize( this, vecMins, vecMaxs );
	
	SetNavType(NAV_GROUND);
	m_NPCState				= NPC_STATE_NONE;
	SetBloodColor( BLOOD_COLOR_YELLOW );
	m_iHealth				= sk_antliongrub_health.GetFloat();
	m_iMaxHealth			= m_iHealth;
	m_flFieldOfView			= -1.0f;
	
	SetSolid( SOLID_BBOX );
	AddSolidFlags( FSOLID_NOT_STANDABLE );

	SetMoveType( MOVETYPE_STEP );

	CapabilitiesClear();
	CapabilitiesAdd( bits_CAP_MOVE_GROUND | bits_CAP_INNATE_MELEE_ATTACK1 );

	m_flNextVoiceChange		= gpGlobals->curtime;
	m_flSquashTime			= gpGlobals->curtime;
	m_flNearTime			= gpGlobals->curtime;
	m_flHealthTime			= gpGlobals->curtime;
	m_flEnemyHostileTime	= gpGlobals->curtime;

	m_bMoving				= false;
	m_bSquashed				= false;
	m_bSquashValid			= false;
	m_bHealing				= false;

	m_nHealthReserve		= 10;
	
	SetTouch( &CNPC_AntlionGrub::GrubTouch );

	CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();

	CPASAttenuationFilter filter( this );
	m_pMovementSound	= controller.SoundCreate( filter, entindex(), CHAN_BODY, "NPC_Antlion.Movement", 3.9f );
	m_pVoiceSound		= controller.SoundCreate( filter, entindex(), CHAN_VOICE, "NPC_Antlion.Voice", 3.9f );
	m_pHealSound		= controller.SoundCreate( filter, entindex(), CHAN_STATIC, "NPC_Antlion.Heal", 3.9f );

	controller.Play( m_pMovementSound, 0.0f, 100 );
	controller.Play( m_pVoiceSound, 0.0f, 100 );
	controller.Play( m_pHealSound, 0.0f, 100 );

	m_pGlowSprite = CSprite::SpriteCreate( "sprites/blueflare1.vmt", GetLocalOrigin(), false );
	
	Assert( m_pGlowSprite );

	if ( m_pGlowSprite == NULL )
		return;

	m_pGlowSprite->TurnOff();
	m_pGlowSprite->SetTransparency( kRenderGlow, 32, 32, 128, 255, kRenderFxNoDissipation );
	//m_pGlowSprite->SetAttachment( this, 1 );
	m_pGlowSprite->SetScale( 1.0f );

	NPCInit();

	BaseClass::Spawn();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pEvent - 
//-----------------------------------------------------------------------------
void CNPC_AntlionGrub::HandleAnimEvent( animevent_t *pEvent )
{
	switch ( pEvent->event )
	{
	case ANTLIONGRUB_AE_START_SQUIRM:
		{
			float duration = random->RandomFloat( 0.1f, 0.3f );

			CSoundEnvelopeController::GetController().SoundChangePitch( m_pMovementSound, random->RandomInt( 100, 120 ), duration );
			CSoundEnvelopeController::GetController().SoundChangeVolume( m_pMovementSound, random->RandomFloat( 0.6f, 0.8f ), duration );
		}
		break;

	case ANTLIONGRUB_AE_END_SQUIRM:
		{
			float duration = random->RandomFloat( 0.1f, 0.3f );

			CSoundEnvelopeController::GetController().SoundChangePitch( m_pMovementSound, random->RandomInt( 80, 100 ), duration );
			CSoundEnvelopeController::GetController().SoundChangeVolume( m_pMovementSound, random->RandomFloat( 0.0f, 0.1f ), duration );
		}
		break;

	default:
		BaseClass::HandleAnimEvent( pEvent );
		return;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CNPC_AntlionGrub::SelectSchedule( void )
{
	//Don't do anything if we're dead
	if ( m_NPCState == NPC_STATE_DEAD )
		return BaseClass::SelectSchedule();

	//If we've heard someone else squeal, we should too
	if ( HasCondition( COND_ANTLIONGRUB_HEARD_SQUEAL ) || HasCondition( COND_ANTLIONGRUB_BEING_SQUASHED ) )
	{
		m_flEnemyHostileTime = gpGlobals->curtime + ANTLIONGRUB_ENEMY_HOSTILE_TIME;
		return SCHED_SMALL_FLINCH;
	}

	//See if we need to run away from our enemy
	if ( m_flEnemyHostileTime > gpGlobals->curtime )
		return SCHED_ANTLIONGUARD_RETREAT;

	if ( HasCondition( COND_ANTLIONGRUB_IN_HEAL_RANGE ) )
	{
		SetTarget( GetEnemy() );
		return SCHED_ANTLIONGRUB_GIVE_HEALTH;
	}

	//If we've taken any damage, squirm and squeal
	if ( HasCondition( COND_LIGHT_DAMAGE ) && SelectWeightedSequence( ACT_SMALL_FLINCH ) != -1 )
		return SCHED_SMALL_FLINCH;

	//Randomly stand still
	if ( random->RandomInt( 0, 3 ) == 0 )
		return SCHED_IDLE_STAND;

	//Otherwise just walk around a little
	return SCHED_PATROL_WALK;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pTask - 
//-----------------------------------------------------------------------------
void CNPC_AntlionGrub::StartTask( const Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_ANTLIONGRUB_FIND_RETREAT_GOAL:
		{
			if ( GetEnemy() == NULL )
			{
				TaskFail( FAIL_NO_ENEMY );
				return;
			}

			Vector	testPos, testPos2, threatDir;
			trace_t	tr;

			//Find the direction to our enemy
			threatDir = ( GetAbsOrigin() - GetEnemy()->GetAbsOrigin() );
			VectorNormalize( threatDir );

			//Find a position farther out away from our enemy
			VectorMA( GetAbsOrigin(), random->RandomInt( 32, 128 ), threatDir, testPos );
			testPos[2] += StepHeight()*2.0f;
			
			testPos2 = testPos;
			testPos2[2] -= StepHeight()*2.0f;

			//Check the position
			AI_TraceLine( testPos, testPos2, MASK_SOLID, this, COLLISION_GROUP_NONE, &tr );

			//Must be clear
			if ( ( tr.startsolid ) || ( tr.allsolid ) || ( tr.fraction == 1.0f ) )
			{
				TaskFail( FAIL_NO_ROUTE );
				return;
			}

			//Save the position and go
			m_vSavePosition = tr.endpos;
			TaskComplete();
		}
		break;

	case TASK_ANTLIONGRUB_MOVE_TO_TARGET:

		if ( GetEnemy() == NULL)
		{
			TaskFail( FAIL_NO_TARGET );
		}
		else if ( ( GetEnemy()->GetLocalOrigin() - GetLocalOrigin()).Length() < pTask->flTaskData )
		{
			TaskComplete();
		}

		break;

	case TASK_ANTLIONGRUB_GIVE_HEALTH:
		
		m_bHealing = true;

		SetActivity( (Activity) ACT_ANTLIONGRUB_HEAL );

		CSoundEnvelopeController::GetController().SoundChangeVolume( m_pHealSound, 0.5f, 2.0f );

		//Must have a target
		if ( GetEnemy() == NULL )
		{
			TaskFail( FAIL_NO_ENEMY );
			return;
		}

		//Must be within range
		if ( (GetEnemy()->GetLocalOrigin() - GetLocalOrigin()).Length() > 92 )
		{
			TaskFail( FAIL_NO_ENEMY );
		}

		break;

	case TASK_ANTLIONGRUB_SQUIRM:
		{
			//Pick a squirm movement to perform
			Vector	vecStart;

			//Move randomly around, and start a step's height above our current position
			vecStart.Random( -32.0f, 32.0f );
			vecStart[2] = StepHeight();
			vecStart += GetLocalOrigin();

			//Look straight down for the ground
			Vector	vecEnd = vecStart;
			vecEnd[2] -= StepHeight()*2.0f;

			trace_t	tr;

			//Check the position
			//FIXME: Trace by the entity's hull size?
			AI_TraceLine( vecStart, vecEnd, MASK_NPCSOLID, this, COLLISION_GROUP_NONE, &tr );

			//See if we can move there
			if ( ( tr.fraction == 1.0f ) || ( tr.startsolid ) || ( tr.allsolid ) )
			{
				TaskFail( FAIL_NO_ROUTE );
				return;
			}

			m_vSavePosition = tr.endpos;
			
			TaskComplete();
		}
		break;

	default:
		BaseClass::StartTask( pTask );
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pTask - 
//-----------------------------------------------------------------------------
void CNPC_AntlionGrub::RunTask( const Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_ANTLIONGRUB_MOVE_TO_TARGET:
		{
			//Must have a target entity
			if ( GetEnemy() == NULL )
			{
				TaskFail( FAIL_NO_TARGET );
				return;
			}

			float distance = ( GetNavigator()->GetGoalPos() - GetLocalOrigin() ).Length2D();
			
			if ( ( GetNavigator()->GetGoalPos() - GetEnemy()->GetLocalOrigin() ).Length() > (pTask->flTaskData * 0.5f) )
			{
				distance = ( GetEnemy()->GetLocalOrigin() - GetLocalOrigin() ).Length2D();
				GetNavigator()->UpdateGoalPos( GetEnemy()->GetLocalOrigin() );
			}

			//See if we've arrived
			if ( distance < pTask->flTaskData )
			{
				TaskComplete();
				GetNavigator()->StopMoving();
			}
		}
		break;

	case TASK_ANTLIONGRUB_GIVE_HEALTH:
		
		//Validate the enemy
		if ( GetEnemy() == NULL )
		{
			TaskFail( FAIL_NO_ENEMY );
			return;
		}

		//Are we done giving health?
		if ( ( GetEnemy()->m_iHealth == GetEnemy()->m_iMaxHealth ) || ( m_nHealthReserve <= 0 ) || ( (GetEnemy()->GetLocalOrigin() - GetLocalOrigin()).Length() > 64 ) )
		{
			m_bHealing = false;
			CSoundEnvelopeController::GetController().SoundChangeVolume( m_pHealSound, 0.0f, 0.5f );
			TaskComplete();
			return;
		}

		//Is it time to heal again?
		if ( m_flHealthTime < gpGlobals->curtime )
		{
			m_flHealthTime = gpGlobals->curtime + 0.5f;
			
			//Update the health
			if ( GetEnemy()->m_iHealth < GetEnemy()->m_iMaxHealth )
			{
				GetEnemy()->m_iHealth++;
				m_nHealthReserve--;
			}
		}

		break;

	default:
		BaseClass::RunTask( pTask );
		break;
	}
}

#define TRANSLATE_SCHEDULE( type, in, out ) { if ( type == in ) return out; }

//-----------------------------------------------------------------------------
// Purpose: override/translate a schedule by type
// Input  : Type - schedule type
// Output : int - translated type
//-----------------------------------------------------------------------------
int CNPC_AntlionGrub::TranslateSchedule( int type ) 
{
	TRANSLATE_SCHEDULE( type, SCHED_IDLE_STAND,	SCHED_ANTLIONGRUB_STAND );
	TRANSLATE_SCHEDULE( type, SCHED_PATROL_WALK,SCHED_ANTLIONGRUB_SQUIRM );

	return BaseClass::TranslateSchedule( type );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pOther - 
//-----------------------------------------------------------------------------
void CNPC_AntlionGrub::GrubTouch( CBaseEntity *pOther )
{
	//Don't consider the world
	if ( FClassnameIs( pOther, "worldspawn" ) )
		return;

	//Allow a crusing velocity to kill them in one go (or they're already dead)
	if ( ( pOther->GetAbsVelocity()[2] < -200 ) || ( IsAlive() == false ) )
	{
		Squash( pOther );
		return;
	}

	//Otherwise, the pressure must be maintained to squash
	if ( m_bSquashValid == false )
	{
		//Shake the screen
		UTIL_ScreenShake( GetAbsOrigin(), 0.01f, 50.0f, 0.1f, 128, SHAKE_START );

		m_bSquashValid = true;
		m_flSquashTime = gpGlobals->curtime + ANTLIONGRUB_SQUASH_TIME;
		
		CSoundEnvelopeController::GetController().CommandClear( m_pVoiceSound );
		CSoundEnvelopeController::GetController().SoundChangePitch( m_pVoiceSound, 150, ANTLIONGRUB_SQUASH_TIME*0.5f );
		CSoundEnvelopeController::GetController().SoundChangeVolume( m_pVoiceSound, 1.0f, 0.5f );
	}
	else
	{
		//Wiggle faster the longer the player is squashing us
		m_flPlaybackRate = 3.0f * ( 1.0f - ( m_flSquashTime - gpGlobals->curtime ) );
		
		//Shake the screen for effect
		UTIL_ScreenShake( GetAbsOrigin(), 1.5f, 0.1f, 0.5f, 128, SHAKE_START );

		//Taken enough damage?
		if ( m_flSquashTime < gpGlobals->curtime )
		{
			EmitSound( "NPC_AntlionGrub.Scared" );
			Squash( pOther );
			return;
		}
	}

	//Need to know we're being squashed
	SetCondition( COND_ANTLIONGRUB_BEING_SQUASHED );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pOther - 
//-----------------------------------------------------------------------------
void CNPC_AntlionGrub::EndTouch( CBaseEntity *pOther )
{
	if(!IsAlive())
	{
		return;
	}
	ClearCondition( COND_ANTLIONGRUB_BEING_SQUASHED );

	m_bSquashValid = false;
	
	CSoundEnvelopeController::GetController().SoundChangePitch( m_pVoiceSound, 100, 0.5f );
	CSoundEnvelopeController::GetController().SoundChangeVolume( m_pVoiceSound, 0.0f, 1.0f );

	m_flPlaybackRate = 1.0f;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_AntlionGrub::BroadcastAlert( void )
{
	CBaseEntity *pEntity = NULL;
	CAI_BaseNPC *pNPC;

	//Look in a radius for potential listeners
	for ( CEntitySphereQuery sphere( GetAbsOrigin(), ANTLIONGRUB_SQUEAL_DIST ); pEntity = sphere.GetCurrentEntity(); sphere.NextEntity() )
	{
		if ( !( pEntity->GetFlags() & FL_NPC ) )
			continue;

		pNPC = pEntity->MyNPCPointer();

		//Only antlions care
		if ( pNPC->Classify() == CLASS_ANTLION )
		{
			pNPC->DispatchInteraction( g_interactionAntlionGrubAlert, NULL, this );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_AntlionGrub::PainSound( void )
{
	BroadcastAlert();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : interactionType - 
//			*data - 
//			*sourceEnt - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CNPC_AntlionGrub::HandleInteraction( int interactionType, void *data, CBaseCombatCharacter *sourceEnt )
{
	//Handle squeals from our peers
	if ( interactionType == g_interactionAntlionGrubAlert )
	{
		SetCondition( COND_ANTLIONGRUB_HEARD_SQUEAL );
		
		//float envDuration = PlayEnvelope( m_pVoiceSound, SOUNDCTRL_CHANGE_VOLUME, envScared, ARRAYSIZE(envScared) );
		float envDuration = CSoundEnvelopeController::GetController().SoundPlayEnvelope( m_pVoiceSound, SOUNDCTRL_CHANGE_VOLUME, envMidSustain, ARRAYSIZE(envMidSustain) );
		m_flNextVoiceChange = gpGlobals->curtime + envDuration + random->RandomFloat( 4.0f, 8.0f );

		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_AntlionGrub::Squash( CBaseEntity *pOther )
{
	SpawnSquashedGrub();

	AddEffects( EF_NODRAW );
	AddSolidFlags( FSOLID_NOT_SOLID );
	
	Event_Killed( CTakeDamageInfo( pOther, pOther, 200, DMG_CRUSH ) );

	EmitSound( "NPC_AntlionGrub.Squash" );
	
	BroadcastAlert();

	trace_t	tr;
	Vector vecDir( 0, 0, -1.0f );

	tr.endpos = GetLocalOrigin();
	tr.endpos[2] += 8.0f;

	MakeDamageBloodDecal( 4, 0.8f, &tr, vecDir );

	SetTouch( NULL );

	m_bSquashed = true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_AntlionGrub::SpawnSquashedGrub( void )
{
	CGib *pStandin = CREATE_ENTITY( CGib, "gib" );

	Assert( pStandin );

	pStandin->SetModel( ANTLIONGRUB_SQUASHED_MODEL );
	pStandin->AddSolidFlags( FSOLID_NOT_SOLID );
	pStandin->SetLocalAngles( GetLocalAngles() );
	pStandin->SetLocalOrigin( GetLocalOrigin() );
}


void CNPC_AntlionGrub::StopLoopingSounds()
{
	CSoundEnvelopeController::GetController().SoundDestroy( m_pMovementSound );
	CSoundEnvelopeController::GetController().SoundDestroy( m_pVoiceSound );
	CSoundEnvelopeController::GetController().SoundDestroy( m_pHealSound );
	m_pMovementSound = NULL;
	m_pVoiceSound = NULL;
	m_pHealSound = NULL;

	BaseClass::StopLoopingSounds();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pInflictor - 
//			*pAttacker - 
//			flDamage - 
//			bitsDamageType - 
//-----------------------------------------------------------------------------
void CNPC_AntlionGrub::Event_Killed( const CTakeDamageInfo &info )
{
	BaseClass::Event_Killed( info );

	m_pGlowSprite->TurnOff();

	if ( m_bSquashed == false )
	{
		//Restore this touch so we can still be squished
		SetTouch(&CNPC_AntlionGrub::GrubTouch );
	}
	else
	{
		Vector	vecMins, vecMaxs;

		vecMins = GetHullMins();
		vecMaxs = GetHullMaxs();

		//Move the bounds out a little
		vecMins[0] -= 4.0f;
		vecMins[1] -= 4.0f;
		vecMaxs[0] += 4.0f;
		vecMaxs[1] += 4.0f;

		//Just the perfect size for being stepped on
		vecMaxs[2] -= 14.0f;

		UTIL_SetSize( this, vecMins, vecMaxs );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_AntlionGrub::PrescheduleThink( void )
{
	BaseClass::PrescheduleThink();
	
	//Add a fail-safe for the glow sprite display
	if ( !IsCurSchedule( SCHED_ANTLIONGRUB_GIVE_HEALTH ) )
	{
		if ( m_bHealing )
		{
			CSoundEnvelopeController::GetController().SoundChangeVolume( m_pHealSound, 0.0f, 0.5f );
		}

		m_bHealing = false;
	}

	//Do glowing maintenance
	if ( m_pGlowSprite != NULL )
	{
		m_pGlowSprite->SetLocalOrigin( GetLocalOrigin() );

		if ( m_pGlowSprite->IsEffectActive( EF_NODRAW ) == false )
		{
			m_pGlowSprite->SetScale( random->RandomFloat( 0.75f, 1.0f ) );
			
			float scale = random->RandomFloat( 0.25f, 0.75f );
			
			m_pGlowSprite->SetTransparency( kRenderGlow, (int)(32.0f*scale), (int)(32.0f*scale), (int)(128.0f*scale), 255, kRenderFxNoDissipation );
		}

		//Deal with the healing glow
		if ( m_bHealing )
		{
			m_pGlowSprite->TurnOn();
		}
		else
		{
			m_pGlowSprite->TurnOff();
		}
	}

	//Check for movement sounds
	if ( m_flGroundSpeed > 0.0f )
	{
		if ( m_bMoving == false )
		{
			CSoundEnvelopeController::GetController().SoundChangePitch( m_pMovementSound, 100, 0.1f );
			CSoundEnvelopeController::GetController().SoundChangeVolume( m_pMovementSound, 0.4f, 1.0f );

			m_bMoving = true;
		}
	}
	else if ( m_bMoving )
	{
		CSoundEnvelopeController::GetController().SoundChangePitch( m_pMovementSound, 80, 0.5f );
		CSoundEnvelopeController::GetController().SoundChangeVolume( m_pMovementSound, 0.0f, 1.0f );
		
		m_bMoving = false;
	}

	//Check for a voice change
	if ( m_flNextVoiceChange < gpGlobals->curtime )
	{
		float envDuration = CSoundEnvelopeController::GetController().SoundPlayEnvelope( m_pVoiceSound, SOUNDCTRL_CHANGE_VOLUME, &grubVoiceEnvelopes[rand()%ARRAYSIZE(grubVoiceEnvelopes)] );
		m_flNextVoiceChange = gpGlobals->curtime + envDuration + random->RandomFloat( 1.0f, 8.0f );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : flDot - 
//			flDist - 
// Output : int
//-----------------------------------------------------------------------------
int CNPC_AntlionGrub::MeleeAttack1Conditions( float flDot, float flDist )
{
	ClearCondition( COND_ANTLIONGRUB_IN_HEAL_RANGE );

	//If we're outside the heal range, then reset our timer
	if ( flDist > ANTLIONGRUB_HEAL_RANGE )
	{
		m_flNearTime = gpGlobals->curtime + 2.0f;
		return COND_TOO_FAR_TO_ATTACK;
	}
	
	//Otherwise if we've been in range for long enough signal it
	if ( m_flNearTime < gpGlobals->curtime )
	{
		if ( ( m_nHealthReserve > 0 ) && ( GetEnemy()->m_iHealth < GetEnemy()->m_iMaxHealth ) )
		{
			SetCondition( COND_ANTLIONGRUB_IN_HEAL_RANGE );
		}
	}

	return COND_CAN_MELEE_ATTACK1;
}

//-----------------------------------------------------------------------------
// Purpose: Allows for modification of the interrupt mask for the current schedule.
//			In the most cases the base implementation should be called first.
//-----------------------------------------------------------------------------
void CNPC_AntlionGrub::BuildScheduleTestBits( void )
{
	//Always squirm if we're being squashed
	if ( !IsCurSchedule( SCHED_SMALL_FLINCH ) )
	{
		SetCustomInterruptCondition( COND_ANTLIONGRUB_BEING_SQUASHED );
	}
}


//-----------------------------------------------------------------------------
//
// Schedules
//
//-----------------------------------------------------------------------------


//==================================================
// SCHED_ANTLIONGRUB_SQUEAL
//==================================================

AI_DEFINE_SCHEDULE
(
	SCHED_ANTLIONGRUB_SQUEAL,

	"	Tasks"
	"		TASK_FACE_ENEMY	0"
	"	"
	"	Interrupts"
	"		COND_ANTLIONGRUB_BEING_SQUASHED	"
	"		COND_NEW_ENEMY"
);

//==================================================
// SCHED_ANTLIONGRUB_STAND
//==================================================

AI_DEFINE_SCHEDULE
(
	SCHED_ANTLIONGRUB_STAND,

	"	Tasks"
	"		TASK_PLAY_SEQUENCE			ACTIVITY:ACT_IDLE"
	"	"
	"	Interrupts"
	"		COND_LIGHT_DAMAGE"
	"		COND_ANTLIONGRUB_HEARD_SQUEAL"
	"		COND_ANTLIONGRUB_BEING_SQUASHED"
	"		COND_NEW_ENEMY"
);

//==================================================
// SCHED_ANTLIONGRUB_SQUIRM
//==================================================

AI_DEFINE_SCHEDULE
(
	SCHED_ANTLIONGRUB_SQUIRM,

	"	Tasks"
	"		TASK_ANTLIONGRUB_SQUIRM	0"
	"		TASK_SET_GOAL			GOAL:SAVED_POSITION"
	"		TASK_GET_PATH_TO_GOAL	PATH:TRAVEL"
	"		TASK_WALK_PATH			0"
	"		TASK_WAIT_FOR_MOVEMENT	0"
	"	"
	"	Interrupts"
	"		COND_TASK_FAILED"
	"		COND_LIGHT_DAMAGE"
	"		COND_ANTLIONGRUB_HEARD_SQUEAL"
	"		COND_ANTLIONGRUB_BEING_SQUASHED"
	"		COND_NEW_ENEMY"
);

//==================================================
// SCHED_ANTLIONGRUB_GIVE_HEALTH
//==================================================

AI_DEFINE_SCHEDULE
(
	SCHED_ANTLIONGRUB_GIVE_HEALTH,

	"	Tasks"
	"		TASK_SET_FAIL_SCHEDULE			SCHEDULE:SCHED_ANTLIONGRUB_STAND"
	"		TASK_STOP_MOVING				0"
	"		TASK_SET_GOAL					GOAL:ENEMY"
	"		TASK_GET_PATH_TO_GOAL			PATH:TRAVEL"
	"		TASK_RUN_PATH					0"
	"		TASK_ANTLIONGRUB_MOVE_TO_TARGET	48"
	"		TASK_STOP_MOVING				0"
	"		TASK_FACE_ENEMY					0"
	"		TASK_ANTLIONGRUB_GIVE_HEALTH	0"
	"		TASK_SET_SCHEDULE				SCHEDULE:SCHED_ANTLIONGRUB_SQUIRM"
	"		"
	"	Interrupts"
	"		COND_TASK_FAILED"
	"		COND_NEW_ENEMY"
	"		COND_ANTLIONGRUB_BEING_SQUASHED"
	"		COND_ANTLIONGRUB_HEARD_SQUEAL"
);

//==================================================
// SCHED_ANTLIONGUARD_RETREAT
//==================================================

AI_DEFINE_SCHEDULE
(
	SCHED_ANTLIONGUARD_RETREAT,

	"	Tasks"
	"		TASK_SET_FAIL_SCHEDULE				SCHEDULE:SCHED_ANTLIONGRUB_STAND"
	"		TASK_STOP_MOVING					0"
	"		TASK_ANTLIONGRUB_FIND_RETREAT_GOAL	0"
	"		TASK_SET_GOAL						GOAL:SAVED_POSITION"
	"		TASK_GET_PATH_TO_GOAL				PATH:TRAVEL"
	"		TASK_RUN_PATH						0"
	"		TASK_WAIT_FOR_MOVEMENT				0"
	"		TASK_SET_SCHEDULE					SCHEDULE:SCHED_ANTLIONGRUB_STAND"
	"	"
	"	Interrupts"
	"		COND_TASK_FAILED"
	"		COND_NEW_ENEMY"
	"		COND_ANTLIONGRUB_BEING_SQUASHED"
	"		COND_ANTLIONGRUB_HEARD_SQUEAL"
);
