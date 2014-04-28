#include "pch.h"
#include "Game.h"
#include "KamilLelonekFSMActorController.h"
#include "contrib/DebugDrawer.h"
#include "Level.h"

KamilLelonekFSMActorController::KamilLelonekFSMActorController( ActorAI* ai ) : StateMachineActorController(ai) {}

void KamilLelonekFSMActorController::onCreate()
{
    getAI() -> lookAt(mkVec3::ZERO);							// define looking vector for actor
    scheduleTransitionInNextFrame(new fsm::IdleState(this));	// prepare idle state as a next state
    updateStateTransition();									// transform into idle state
}

void KamilLelonekFSMActorController::onDebugDraw()
{
    __super::onDebugDraw();
}

fsm::BaseState::BaseState( KamilLelonekFSMActorController* controller ) : sm::State(controller) {}
fsm::IdleState::IdleState( KamilLelonekFSMActorController* controller ) : fsm::BaseState(controller), m_stateStartTime(-1.f) {}
fsm::AttackState::AttackState( KamilLelonekFSMActorController* controller, Character* target ) : fsm::BaseState(controller), m_target(target), m_stateStartTime(-1.f) {}

KamilLelonekFSMActorController* fsm::BaseState::getController() const
{
    return static_cast<KamilLelonekFSMActorController*>(sm::State::getController());
}

void fsm::IdleState::onUpdate(float dt)
{
    __super::onUpdate(dt);

    Character* target = getAI()->findClosestEnemyInSight();
    if(target){
        getController()->scheduleTransitionInNextFrame(new fsm::AttackState(getController(),target));
    }

    const float time_to_change_state = 2000;
    const float cur_time = g_game->getTimeMs();
    if (cur_time - m_stateStartTime > time_to_change_state)
    {
        fsm::IdleState* next_state = new fsm::IdleState(getController());
        getController()->scheduleTransitionInNextFrame(next_state);
    }
}

void fsm::IdleState::onEnter(State* prev_state)
{
    __super::onEnter(prev_state);
    m_stateStartTime = g_game->getTimeMs();
    getAI()->setSpeed(0.f);
}

void fsm::IdleState::onTakeDamage()
{
    getAI()->jump();
}

void fsm::IdleState::onDebugDraw() {}

void fsm::IdleState::onLeave( State* next_state )
{
    getAI()->runAnimation("Backflip",800);
}

void fsm::AttackState::onUpdate(float dt)
{
    __super::onUpdate(dt);

    Character* target = getAI()->findClosestEnemyInSight();
    if(!target){
        getController()->scheduleTransitionInNextFrame(new fsm::IdleState(getController()));
    }

    const float time_to_change_state = 1200;
    const float cur_time = g_game->getTimeMs();
    if (cur_time - m_stateStartTime > time_to_change_state)
    {
        getAI()->runAnimation("Attack3",time_to_change_state);
        fsm::AttackState* next_state = new fsm::AttackState(getController(),target);
        getController()->scheduleTransitionInNextFrame(next_state);
    }
}

void fsm::AttackState::onEnter(State* prev_state)
{
    __super::onEnter(prev_state);
    m_stateStartTime = g_game->getTimeMs();
    getAI()->setSpeed(0.f);
}

void fsm::AttackState::onLeave( State* next_state )
{
    getAI()->hitMelee();
}
