#pragma once
#include "StateMachineActorController.h"

class KamilLelonekFSMActorController : public StateMachineActorController
{
public:
    explicit KamilLelonekFSMActorController(ActorAI* ai);

    virtual void onCreate();
    virtual void onDebugDraw();
};

namespace fsm
{
    class BaseState : public sm::State
    {
    public:
        explicit BaseState(KamilLelonekFSMActorController* controller);

        KamilLelonekFSMActorController* getController() const;
    };

    class IdleState : public BaseState
    {
    public:
        IdleState(KamilLelonekFSMActorController* controller);

        void onUpdate(float dt);
        void onEnter(State* prev_state);

        virtual void onTakeDamage();

        virtual void onDebugDraw();

        virtual void onLeave( State* next_state );

    private:
        float m_stateStartTime;
    };

    class AttackState : public BaseState
    {
    public:
        AttackState(KamilLelonekFSMActorController* controller, Character* target);

        void onUpdate(float dt);
        void onEnter(State* prev_state);

        virtual void onLeave( State* next_state );

    private:
        Character* m_target;
        float m_stateStartTime;
    };
}
