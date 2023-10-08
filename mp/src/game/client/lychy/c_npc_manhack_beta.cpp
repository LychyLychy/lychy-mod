//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "c_AI_BaseNPC.h"

class C_NPC_ManhackBeta : public C_AI_BaseNPC
{
public:
	C_NPC_ManhackBeta() {}

	DECLARE_CLASS( C_NPC_ManhackBeta, C_AI_BaseNPC );
	DECLARE_CLIENTCLASS();

private:
	C_NPC_ManhackBeta( const C_NPC_ManhackBeta & );
};


IMPLEMENT_CLIENTCLASS_DT(C_NPC_ManhackBeta, DT_NPC_ManhackBeta, CNPC_ManhackBeta)
END_RECV_TABLE()