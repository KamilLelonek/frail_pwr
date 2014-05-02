/***********************************
 * mkdemo 2011-2013                *
 * author: Maciej Kurowski 'kurak' *
 ***********************************/
#include "pch.h"
#include "ActorControllerFactory.h"

//KamilLelonek///////////////////////////////////////////////////////////////
#include "KamilLelonekFSMActorController.h"
#include "KamilLelonekBTActorController.h"
#include "KamilLelonekHTNActorController.h"

// TODO: remove this factory, make actor controllers ordinary GameObjects and use RTTI for instantiating controllers dynamically with their class name

#define CREATE_CONTROLLER(class_prefix) if (are_strings_equal_case_insensitive(controller_id, #class_prefix)) { \
                                            IActorController* result = new class_prefix##ActorController(ai);   \
                                            m_createdControllers.push_back(result);                             \
                                            result -> onCreate();                                               \
                                            return result;                                                      \
                                        }

IActorController* ActorControllerFactory::create( const mkString& controller_id, ActorAI* ai )
{
    CREATE_CONTROLLER(KamilLelonekFSM);
    CREATE_CONTROLLER(KamilLelonekBT);
    CREATE_CONTROLLER(KamilLelonekHTN);

    MK_ASSERT_MSG(false, "Unrecognized actor controller '%s'", controller_id.c_str());
    return NULL;
}

#undef CREATE_CONTROLLER

void ActorControllerFactory::release( IActorController* controller )
{
    MK_ASSERT(!controller->getAI());

    delete controller;
    m_createdControllers.erase(std::remove(m_createdControllers.begin(), m_createdControllers.end(), controller), m_createdControllers.end());
}

void ActorControllerFactory::releaseAll()
{
    for (size_t i = 0; i < m_createdControllers.size(); ++i)
        delete m_createdControllers[i];

    m_createdControllers.clear();
}
