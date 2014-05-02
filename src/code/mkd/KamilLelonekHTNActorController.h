#pragma once
#include "IActorController.h"
#include "Player.h"
#include "utils.h"
#include "HTN\Planner.h"

class KamilLelonekHTNActorController : public IActorController
{
public:
    typedef bool (KamilLelonekHTNActorController::*ctrlrAction)(float);

	explicit KamilLelonekHTNActorController(ActorAI* ai);
	~KamilLelonekHTNActorController();

	virtual void onCreate();
	virtual void onTakeDamage(const SDamageInfo& dmg_info);
	virtual void onUpdate(float dt);
	virtual void onDebugDraw();
    virtual void onDie();

private:
    std::map<std::string, ctrlrAction> m_actions;
    HTN::Planner *m_planner;
    Character *m_target;

    mkVec3 m_enemyLastPos;
    mkVec3 m_attackDir;

    bool m_isAttacked;
    bool m_enemyRunningAway;
    bool m_angerMode;

    float m_prevEnemyDist;
    float m_prevDistSum;
    float m_prevEnemyDistTime;

    void updateWorldState(float dt);
    void executeTask(HTN::pOperator op);

	bool isActorHealthy();
	
	/*********************
			ACTIONS
	*********************/
	bool actionPatrol			(float duration);
    bool actionRotateToEnemy	(float duration);
	bool actionAttackMelee		(float duration);
	bool actionAttackFireball	(float duration);
	bool actionReduceDistance	(float duration);
	bool actionRevealAttacker	(float duration);
    bool actionAngerMode		(float duration);
    bool actionExploreSpot		(float duration);
    bool actionKeepDistance		(float duration);
    bool actionHeal				(float duration);

	/*********************
		  ANIMATIONS
	*********************/
    bool animAngerMode			(float duration);
    bool animAttackMelee		(float duration);
    bool animAttackFireball		(float duration);
};