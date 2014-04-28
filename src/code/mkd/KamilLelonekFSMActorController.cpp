#include "pch.h"
#include "Game.h"
#include "KamilLelonekFSMActorController.h"
#include "contrib/DebugDrawer.h"
#include "Level.h"

/*
	CONTROLLER
*/
KamilLelonekFSMActorController::KamilLelonekFSMActorController( ActorAI* ai ) : StateMachineActorController(ai) {}

void KamilLelonekFSMActorController::onCreate()
{
    scheduleTransitionInNextFrame(new kamillelonek_fsm::PatrolState(this));
    updateStateTransition();
}

/*
	STATES
*/
kamillelonek_fsm::BaseState::BaseState( KamilLelonekFSMActorController* controller ) : sm::State(controller), m_stateStartTime(-1.f) {}

kamillelonek_fsm::PatrolState::PatrolState( KamilLelonekFSMActorController* controller ) : kamillelonek_fsm::BaseState(controller) {}
kamillelonek_fsm::AttackState::AttackState( KamilLelonekFSMActorController* controller, Character* target ) : kamillelonek_fsm::BaseState(controller), m_target(target) {}
kamillelonek_fsm::SickState::SickState( KamilLelonekFSMActorController * controller) : kamillelonek_fsm::BaseState(controller) {}

/*
	STATE BASE
*/
KamilLelonekFSMActorController* kamillelonek_fsm::BaseState::getController() const
{
    return static_cast<KamilLelonekFSMActorController*>(sm::State::getController());
}

void kamillelonek_fsm::BaseState::onEnter(State* prev_state)
{
	m_stateStartTime = g_game -> getTimeMs();
}

void kamillelonek_fsm::BaseState::onUpdate(float dt)
{
    __super::onUpdate(dt);

    const float time_to_change_state = 2000;
    const float cur_time = g_game -> getTimeMs();
    if (cur_time - m_stateStartTime < time_to_change_state) return;

	if(shouldBeHealed())
	{
		getController() -> scheduleTransitionInNextFrame(new kamillelonek_fsm::SickState(getController()));
	}
	else if(enemyInRange())
	{
		
	}
	else
	{
		getController() -> scheduleTransitionInNextFrame(new kamillelonek_fsm::PatrolState(getController()));
	}
}

bool kamillelonek_fsm::BaseState::shouldBeHealed()
{

	return (getAI() -> getHealth()) < (getAI() -> getMaxHealth()) / 2;
}

bool kamillelonek_fsm::BaseState::enemyInRange()
{
	Character* target = getAI()->findClosestEnemyInSight();
	if(target)
		throw std::exception();
	return false;
}

/*
	STATE PATROL
*/
void kamillelonek_fsm::PatrolState::onEnter(State* prev_state)
{
    __super::onEnter(prev_state);

    getAI() -> lookAt(getRandomHorizontalDir());
    getAI() -> setSpeed(0.5f);
}

void kamillelonek_fsm::PatrolState::onUpdate(float dt)
{
    __super::onUpdate(dt);

    Character* target = getAI() -> findClosestEnemyInSight();
    if(target)
	{
        getController() -> scheduleTransitionInNextFrame(new kamillelonek_fsm::AttackState(getController(), target));
    }
}

void kamillelonek_fsm::PatrolState::onTakeDamage()
{
    getAI() -> runAnimation("Backflip", 800);
}

/*
	STATE SICK
*/
void kamillelonek_fsm::SickState::onEnter(State* prev_state)
{
    __super::onEnter(prev_state);
	ActorAI* actor = getAI();
    actor -> lookAt(actor -> getPowerLakePosition());
    actor -> setSpeed(1.f);
}

void kamillelonek_fsm::SickState::onUpdate(float dt)
{
    __super::onUpdate(dt);
	if(getAI() -> isInPowerLake()) getAI() -> setSpeed(0.f);
}

/*
	STATE ATTACK
*/
void kamillelonek_fsm::AttackState::onEnter(State* prev_state)
{
    __super::onEnter(prev_state);
    getAI() -> setSpeed(0.f);
}

void kamillelonek_fsm::AttackState::onUpdate(float dt)
{
    __super::onUpdate(dt);

    Character* target = getAI() -> findClosestEnemyInSight();
    if(!target)
	{
        getController() -> scheduleTransitionInNextFrame(new kamillelonek_fsm::PatrolState(getController()));
    }

    //getAI() -> runAnimation("Attack3", 2000.f);
	getAI() -> hitMelee();
    kamillelonek_fsm::AttackState* next_state = new kamillelonek_fsm::AttackState(getController(), target);
    getController() -> scheduleTransitionInNextFrame(new kamillelonek_fsm::AttackState(getController(), target));
}
