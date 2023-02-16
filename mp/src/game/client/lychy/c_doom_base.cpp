#include "cbase.h"
#include "c_ai_basenpc.h"
#include "Sprite.h"

class C_BaseDoom : public  C_AI_BaseNPC, public C_SpriteRenderer
{
	DECLARE_CLASS(C_BaseDoom, C_AI_BaseNPC);
	DECLARE_CLIENTCLASS();
	bool ShouldDraw() { return true; }
	int DrawModel(int flags) override; 
	void Precache();
	void Spawn();
};


IMPLEMENT_CLIENTCLASS_DT(C_BaseDoom, DT_BaseDoom, CBaseDoom)
END_RECV_TABLE()

void C_BaseDoom::Precache()
{
	//const model_t * model = engine->LoadModel("materials/doom/rabblerouser/rabb.vmt");
	PrecacheModel("materials/doom/rabblerouser/rabb.vmt");
}	
void C_BaseDoom::Spawn()
{
	Precache();
	SetModel("materials/doom/rabblerouser/rabb.vmt");
}
int C_BaseDoom::DrawModel(int flags)
{
	C_BasePlayer* pPlayer = C_BasePlayer::GetLocalPlayer();
	Vector vForward;
	GetVectors(&vForward,NULL,NULL);
	Vector thisToPlayer = pPlayer->GetAbsOrigin() - GetAbsOrigin();
	thisToPlayer.z = 0;
	thisToPlayer.NormalizeInPlace();
	
	vec_t dotPr = DotProduct(vForward, thisToPlayer);
	int frame = acos(dotPr) / (M_PI / 5);
	Msg("%f\n", RAD2DEG(acos(dotPr)));
	return DrawSprite(this, GetModel(), WorldSpaceCenter(), vec3_angle, frame, NULL, -1, kRenderTransAlpha, kRenderFxNone, 255, 255, 255, 255, 0.25);
}