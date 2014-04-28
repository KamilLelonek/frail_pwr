#pragma once
#include "StateMachineActorController.h"

class KamilLelonekFSMActorController : public StateMachineActorController
{
public:
    explicit KamilLelonekFSMActorController(ActorAI* ai);
	virtual void onCreate();
};

namespace kamillelonek_fsm
{
    class BaseState : public sm::State
    {
    public:
        explicit BaseState(KamilLelonekFSMActorController* controller);
		KamilLelonekFSMActorController* getController() const;

        void onUpdate(float dt);
        void onEnter(State* prev_state);

		bool shouldBeHealed();
		bool enemyInRange();

    private:
        float m_stateStartTime;
    };

    class PatrolState : public BaseState
    {
    public:
        PatrolState(KamilLelonekFSMActorController* controller);

        void onUpdate(float dt);
        void onEnter(State* prev_state);

        virtual void onTakeDamage();
    };

    class AttackState : public BaseState
    {
    public:
        AttackState(KamilLelonekFSMActorController* controller, Character* target);

        void onUpdate(float dt);
        void onEnter(State* prev_state);

    private:
        Character* m_target;
    };

	class SickState : public BaseState
	{
	public:
        SickState(KamilLelonekFSMActorController* controller);
		
        void onUpdate(float dt);
        void onEnter(State* prev_state);

	private:
        mkVec3 m_direction;
	};
}
