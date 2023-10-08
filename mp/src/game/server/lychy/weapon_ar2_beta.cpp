//========= Copyright � 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "basecombatweapon.h"
#include "NPCevent.h"
#include "basecombatcharacter.h"
#include "AI_BaseNPC.h"
#include "player.h"
#include "weapon_ar2_beta.h"
#include "grenade_ar2.h"
#include "gamerules.h"
#include "game.h"
#include "in_buttons.h"
#include "AI_Memory.h"
#include "shake.h"
#include "lychy_convars.h"

extern ConVar    sk_plr_dmg_ar2_grenade;	
extern ConVar    sk_npc_dmg_ar2_grenade;
extern ConVar    sk_max_ar2_grenade;
extern ConVar	 sk_ar2_grenade_radius;

#define AR2_ZOOM_RATE	0.5f	// Interval between zoom levels in seconds.
FACTORYCMD(lychy_ar2_beta, "weapon_ar2", "weapon_ar2_beta");
//=========================================================
//=========================================================

BEGIN_DATADESC( CWeaponAR2Beta )

	DEFINE_FIELD(  m_nShotsFired,	FIELD_INTEGER ),
	DEFINE_FIELD(  m_bZoomed,		FIELD_BOOLEAN ),

END_DATADESC()

IMPLEMENT_SERVERCLASS_ST(CWeaponAR2Beta, DT_WeaponAR2Beta)
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS( weapon_ar2_beta, CWeaponAR2Beta );
PRECACHE_WEAPON_REGISTER(weapon_ar2_beta);

acttable_t	CWeaponAR2Beta::m_acttable[] = 
{
	{ ACT_RANGE_ATTACK1,	ACT_RANGE_ATTACK_AR2, true },
	{ ACT_WALK,				ACT_WALK_RIFLE,					false },
	{ ACT_WALK_AIM,			ACT_WALK_AIM_RIFLE,				false },
	{ ACT_WALK_CROUCH,		ACT_WALK_CROUCH_RIFLE,			false },
	{ ACT_WALK_CROUCH_AIM,	ACT_WALK_CROUCH_AIM_RIFLE,		false },
	{ ACT_RUN,				ACT_RUN_RIFLE,					false },
	{ ACT_RUN_AIM,			ACT_RUN_AIM_RIFLE,				false },
	{ ACT_RUN_CROUCH,		ACT_RUN_CROUCH_RIFLE,			false },
	{ ACT_RUN_CROUCH_AIM,	ACT_RUN_CROUCH_AIM_RIFLE,		false },
	{ ACT_GESTURE_RANGE_ATTACK1,	ACT_GESTURE_RANGE_ATTACK_AR2,	false },
//	{ ACT_RANGE_ATTACK2, ACT_RANGE_ATTACK_AR2_GRENADE, true },
};

IMPLEMENT_ACTTABLE(CWeaponAR2Beta);

CWeaponAR2Beta::CWeaponAR2Beta( )
{
	m_fMinRange1	= 65;
	m_fMaxRange1	= 2048;

	m_fMinRange2	= 256;
	m_fMaxRange2	= 1024;

	m_nShotsFired	= 0;
}

void CWeaponAR2Beta::Precache( void )
{
	UTIL_PrecacheOther("grenade_ar2");
	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose: Offset the autoreload
//-----------------------------------------------------------------------------
bool CWeaponAR2Beta::Deploy( void )
{
	m_nShotsFired = 0;

	return BaseClass::Deploy();
}

//-----------------------------------------------------------------------------
// Purpose: Handle grenade detonate in-air (even when no ammo is left)
//-----------------------------------------------------------------------------
void CWeaponAR2Beta::ItemPostFrame( void )
{
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	if (!pOwner)
		return;

	if ( ( pOwner->m_nButtons & IN_ATTACK ) == false )
	{
		m_nShotsFired = 0;
	}

	//Zoom in
	if ( pOwner->m_afButtonPressed & IN_ATTACK2 )
	{
		Zoom();
	}

	//Don't kick the same when we're zoomed in
	if ( m_bZoomed )
	{
		m_fFireDuration = 0.05f;
	}

	BaseClass::ItemPostFrame();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Activity
//-----------------------------------------------------------------------------
Activity CWeaponAR2Beta::GetPrimaryAttackActivity( void )
{
	if ( m_nShotsFired < 2 )
		return ACT_VM_PRIMARYATTACK;

	if ( m_nShotsFired < 3 )
		return ACT_VM_HITLEFT;
	
	if ( m_nShotsFired < 4 )
		return ACT_VM_HITLEFT2;

	return ACT_VM_HITRIGHT;
}

//---------------------------------------------------------
//---------------------------------------------------------
void CWeaponAR2Beta::PrimaryAttack( void )
{
	m_nShotsFired++;

	BaseClass::PrimaryAttack();
}

void CWeaponAR2Beta::Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator )
{
	switch( pEvent->event )
	{ 
		case EVENT_WEAPON_AR2:
		{
			Vector vecShootOrigin, vecShootDir;
			vecShootOrigin = pOperator->Weapon_ShootPosition();

			CAI_BaseNPC *npc = pOperator->MyNPCPointer();
			ASSERT( npc != NULL );
			vecShootDir = npc->GetActualShootTrajectory( vecShootOrigin );
			WeaponSound(SINGLE_NPC);
			pOperator->FireBullets( 1, vecShootOrigin, vecShootDir, VECTOR_CONE_PRECALCULATED, MAX_TRACE_LENGTH, m_iPrimaryAmmoType, 2 );
			pOperator->m_fEffects |= EF_MUZZLEFLASH;
			m_iClip1 = m_iClip1 - 1;
		}
		break;
		default:
			CBaseCombatWeapon::Operator_HandleAnimEvent( pEvent, pOperator );
			break;
	}
}

/*
==================================================
AddViewKick
==================================================
*/

void CWeaponAR2Beta::AddViewKick( void )
{
	#define	EASY_DAMPEN			0.5f
	#define	MAX_VERTICAL_KICK	24.0f	//Degrees
	#define	SLIDE_LIMIT			3.0f	//Seconds
	
	//Get the view kick
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );

	if (!pPlayer)
		return;

	DoMachineGunKick( pPlayer, EASY_DAMPEN, MAX_VERTICAL_KICK, m_fFireDuration, SLIDE_LIMIT );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponAR2Beta::Zoom( void )
{
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );
	
	if ( pPlayer == NULL )
		return;

	color32 lightGreen = { 50, 255, 170, 32 };

	if ( m_bZoomed )
	{
		pPlayer->ShowViewModel( true );

		// Zoom out to the default zoom level
		WeaponSound(SPECIAL2);
		pPlayer->SetFOV(this, 0, 0.1f );
		m_bZoomed = false;

		UTIL_ScreenFade( pPlayer, lightGreen, 0.2f, 0, (FFADE_IN|FFADE_PURGE) );
	}
	else
	{
		pPlayer->ShowViewModel( false );

		WeaponSound(SPECIAL1);
		pPlayer->SetFOV(this, 35, 0.1f );
		m_bZoomed = true;

		UTIL_ScreenFade( pPlayer, lightGreen, 0.2f, 0, (FFADE_OUT|FFADE_PURGE|FFADE_STAYOUT) );	
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : float
//-----------------------------------------------------------------------------
float CWeaponAR2Beta::GetFireRate( void )
{ 
	if ( m_bZoomed )
		return 0.3f;

	return 0.1f;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : NULL - 
//-----------------------------------------------------------------------------
bool CWeaponAR2Beta::Holster( CBaseCombatWeapon *pSwitchingTo )
{
	if ( m_bZoomed )
	{
		Zoom();
	}

	return BaseClass::Holster( pSwitchingTo );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CWeaponAR2Beta::Reload( void )
{
	if ( m_bZoomed )
	{
		Zoom();
	}

	return BaseClass::Reload();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponAR2Beta::Drop( const Vector &velocity )
{
	if ( m_bZoomed )
	{
		Zoom();
	}

	BaseClass::Drop( velocity );
}

void CWeaponAR2Beta::Spawn()
{
	m_szAltClassName = MAKE_STRING(GetClassname());
	SetClassname("weapon_ar2_beta");
	
	BaseClass::Spawn();
}