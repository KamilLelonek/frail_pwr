#include "pch.h"
#include "Game.h"
#include "KamilLelonekFSMActorController.h"
#include "contrib/DebugDrawer.h"
#include "Level.h"

/******************
	 CONTROLLER
*******************/
KamilLelonekFSMActorController::KamilLelonekFSMActorController( ActorAI* ai ) : StateMachineActorController(ai) {}

void KamilLelonekFSMActorController::onCreate()
{
    scheduleTransitionInNextFrame(new kamillelonek_fsm::PatrolState(this));
    updateStateTransition();
}

/******************
	   STATES
*******************/
kamillelonek_fsm::BaseState::BaseState		( KamilLelonekFSMActorController* controller )						: sm::State(controller), m_stateStartTime(-1.f) {}
kamillelonek_fsm::PatrolState::PatrolState	( KamilLelonekFSMActorController* controller )						: kamillelonek_fsm::BaseState(controller) {}
kamillelonek_fsm::SickState::SickState		( KamilLelonekFSMActorController* controller )						: kamillelonek_fsm::BaseState(controller) {}
kamillelonek_fsm::AttackState::AttackState	( KamilLelonekFSMActorController* controller, Character* target )	: kamillelonek_fsm::BaseState(controller), m_target(target) {}

/******************
	STATE BASE
*******************/
KamilLelonekFSMActorController* kamillelonek_fsm::BaseState::getController() const
{
    return static_cast<KamilLelonekFSMActorController*>(sm::State::getController());
}

void kamillelonek_fsm::BaseState::onEnter(State* prev_state)
{
    __super::onEnter(prev_state);
	m_stateStartTime = g_game -> getTimeMs();
	isUnderAttack = false;
}

void kamillelonek_fsm::BaseState::onUpdate(float dt)
{
    __super::onUpdate(dt);

    const float time_to_change_state = 2000;
    const float cur_time = g_game -> getTimeMs();
    if (cur_time - m_stateStartTime < time_to_change_state) return;

	Character* target;
	if(shouldBeHealed() && !isUnderAttack)
	{
		getController() -> scheduleTransitionInNextFrame(new kamillelonek_fsm::SickState(getController()));
	}
	else if(target = enemyInRange())
	{
		getController() -> scheduleTransitionInNextFrame(new kamillelonek_fsm::AttackState(getController(), target));
	}
	else
	{
		getController() -> scheduleTransitionInNextFrame(new kamillelonek_fsm::PatrolState(getController()));
	}
}

bool kamillelonek_fsm::BaseState::shouldBeHealed()
{
	return (getAI() -> getHealth()) <= (getAI() -> getMaxHealth()) / 2;
}

Character* kamillelonek_fsm::BaseState::enemyInRange()
{
	Character* target = getAI() -> findClosestEnemyInSight();
	return target;
}

void kamillelonek_fsm::BaseState::onTakeDamage()
{
	getAI() -> setDirection(getRandomHorizontalDir());
}

/******************
	STATE PATROL
*******************/
void kamillelonek_fsm::PatrolState::onEnter(State* prev_state)
{
    __super::onEnter(prev_state);

    getAI() -> lookAt(getRandomHorizontalDir());
    getAI() -> setSpeed(0.5f);
}

void kamillelonek_fsm::PatrolState::onUpdate(float dt)
{
    __super::onUpdate(dt);

}

/******************
	STATE SICK
*******************/
void kamillelonek_fsm::SickState::onEnter(State* prev_state)
{
    __super::onEnter(prev_state);
	ActorAI* actor = getAI();
	if(actor -> isMedkitAvailable())
	{
		actor -> lookAt(getAI() -> getMedkitPosition());
	}
	else
	{
		actor -> lookAt(actor -> getPowerLakePosition());
	}
    actor -> setSpeed(10.f);
}

void kamillelonek_fsm::SickState::onUpdate(float dt)
{
    __super::onUpdate(dt);
	if(getAI() -> isInPowerLake()) getAI() -> setSpeed(0.f);
}

/******************
	STATE ATTACK
*******************/
void kamillelonek_fsm::AttackState::onEnter(State* prev_state)
{
    __super::onEnter(prev_state);
	isUnderAttack = true;
    m_stateStartTime = g_game -> getTimeMs();
    getAI() -> setSpeed(0.f);
}

void kamillelonek_fsm::AttackState::onUpdate(float dt)
{
    __super::onUpdate(dt);
	
	mkVec3 enemyPosition = m_target -> getSimPos();
	getAI() -> lookAt(enemyPosition);
	getAI() -> setSpeed(10.f);

	const float time_to_change_state = 2000;
    const float cur_time = g_game -> getTimeMs();
    if (cur_time - m_stateStartTime > time_to_change_state)
    {
		float distance = (getAI() -> getSimPos()).distance(enemyPosition);
		if(distance <= getAI() -> getMeleeRange())
		{
			getAI() -> runAnimation("Attack3", 2000.f);
			getAI() -> hitMelee();
		}
		else if(distance <= getAI() -> getShootingRange())
		{
			getAI() -> hitFireball(enemyPosition);
		}
	}
}

void kamillelonek_fsm::AttackState::onLeave(State* next_state)
{
	isUnderAttack = false;
}
