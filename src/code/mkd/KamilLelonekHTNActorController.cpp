#include "pch.h"
#include "KamilLelonekHTNActorController.h"
#include "contrib/DebugDrawer.h"

KamilLelonekHTNActorController::KamilLelonekHTNActorController( ActorAI* ai ) : IActorController(ai)
{
    m_planner = new HTN::Planner();
    m_planner->init(getAI()->getHtnMethodsPath(),getAI()->getHtnOperatorsPath(),getAI()->getHtnGoalsPath());

    //actions////////////////////////////////////////////////////////////////////////
    m_actions["opPatrol"] = &KamilLelonekHTNActorController::actionPatrol;
    m_actions["opAttackMelee"] = &KamilLelonekHTNActorController::actionAttackMelee;
    m_actions["opRotateToEnemy"] = &KamilLelonekHTNActorController::actionRotateToEnemy;
    m_actions["opAttackFireball"] = &KamilLelonekHTNActorController::actionAttackFireball;
    m_actions["opReduceDistance"] = &KamilLelonekHTNActorController::actionReduceDistance;
    m_actions["opRevealAttacker"] = &KamilLelonekHTNActorController::actionRevealAttacker;
    m_actions["opAngerMode"] = &KamilLelonekHTNActorController::actionAngerMode;
    m_actions["opExploreSpot"] = &KamilLelonekHTNActorController::actionExploreSpot;
    m_actions["opKeepDistance"] = &KamilLelonekHTNActorController::actionKeepDistance;
    //////////////////////////////////////////////////////////////////////////
    m_actions["animAngerMode"] = &KamilLelonekHTNActorController::animAngerMode;
    m_actions["animAttackFireball"] = &KamilLelonekHTNActorController::animAttackFireball;
    m_actions["animAttackMelee"] = &KamilLelonekHTNActorController::animAttackMelee;

    m_isAttacked = false;
    m_enemyRunningAway = false;
    m_prevEnemyDist = 1000.f;
    m_prevEnemyDistTime = 0.f;
    m_prevDistSum = 0.f;
    m_angerMode = false;
    m_enemyLastPos = mkVec3::ZERO;
    m_target = NULL;
}

KamilLelonekHTNActorController::~KamilLelonekHTNActorController(){
    delete m_planner;
}

//////////////////////////////////////////////////////////////////////////

void KamilLelonekHTNActorController::onCreate(){
    getAI()->lookAt(mkVec3::ZERO);
    //ai_specific////////////////////////////////////////////////////////////////////////
    m_planner->setStateFloat("rngMelee",getAI()->getMeleeRange());
    m_planner->setStateFloat("rngFbMax",getAI()->getShootingRange());
    m_planner->setStateFloat("dmgConeSize",getAI()->getMeleeConeSize());
    m_planner->setStateFloat("HealthAMLimit",(getAI()->getMaxHealth())/2);
    //conditions////////////////////////////////////////////////////////////////////////
    m_planner->setStateBool("IsEnemyVisible",false);
    m_planner->setStateBool("IsEnemyDead",false);
    m_planner->setStateBool("IsEnemyAttack",false);
    m_planner->setStateBool("IsEnemyRunningAway",false);
    m_planner->setStateBool("IsActorAM",false);
    m_planner->setStateBool("IsEnemySeen",false);
    m_planner->setStateBool("IsSpotReached",false);
    m_planner->setStateFloat("EnemyDistance",FLT_MAX);
    m_planner->setStateFloat("ActorHealth",0.f);
    m_planner->setStateFloat("EnemyDgrDiff",0.f);

    Ogre::LogManager::getSingleton().logMessage("HTN controller created!");
}

void KamilLelonekHTNActorController::onTakeDamage(const SDamageInfo& dmg_info){
    m_isAttacked = true;
    m_attackDir = mkVec3::ZERO-dmg_info.dir;
}

void KamilLelonekHTNActorController::onUpdate(float dt){
    updateWorldState(dt);

    std::vector<HTN::pTask> plan = m_planner->getPlan(getAI()->getHtnMethodsPath(),getAI()->getHtnOperatorsPath(),getAI()->getHtnGoalsPath());
    HTN::pOperator newTask;
    HTN::PlanResult result = m_planner->resolvePlan(plan, dt, newTask);

    switch (result)
    {
    case HTN::PLAN_EMPTYPLAN:
        getAI()->setSpeed(0.f);
        break;
    case HTN::PLAN_INTERRUPTED:
        getAI()->stopSmoothChangeDir();
        getAI()->stopAnimation();
    case HTN::PLAN_NEW:
        executeTask(newTask);
        break;
    case HTN::PLAN_RUNNING:
        break;
    }
}

void KamilLelonekHTNActorController::onDebugDraw(){
    getAI()->drawSensesInfo();
    if(m_enemyLastPos != mkVec3::ZERO)
        DebugDrawer::getSingleton().drawCircle(m_enemyLastPos, 0.2f, 30, Ogre::ColourValue::Black, true);
}

void KamilLelonekHTNActorController::onDie()
{
    m_angerMode = false;
}

//////////////////////////////////////////////////////////////////////////

void KamilLelonekHTNActorController::updateWorldState(float dt){
    if(m_isAttacked)
        m_planner->setStateBool("IsEnemyAttack",true);
    else
        m_planner->setStateBool("IsEnemyAttack",false);
    
    m_planner->setStateFloat("ActorHealth",getAI()->getHealth());
    m_planner->setStateBool("IsActorAM",m_angerMode);

    m_target = getAI()->findClosestEnemyInSight();
    m_planner->setStateBool("IsEnemyVisible",m_target ? true : false);
    if(m_target){
        float angleDiff = (float)(getAI()->getCharToEnemyAngle(m_target->getSimPos()).valueDegrees());
        m_planner->setStateFloat("EnemyDgrDiff",angleDiff);
        ActorAI *targetAI = dynamic_cast<ActorAI*>(m_target);
        Player *targetPlayer = dynamic_cast<Player*>(m_target);
        if(targetAI)
            m_planner->setStateBool("IsEnemyDead",targetAI->getHealth() > 0.f ? false : true);
        else if(targetPlayer)
            m_planner->setStateBool("IsEnemyDead",targetPlayer->getHealth() > 0.f ? false : true);

        float enemyDistance = (float)(m_target->getSimPos() - getAI()->getSimPos()).length();
        m_planner->setStateFloat("EnemyDistance",enemyDistance);

        m_prevDistSum += enemyDistance - m_prevEnemyDist;
        m_prevEnemyDist = enemyDistance;

        m_prevEnemyDistTime = m_prevEnemyDistTime - dt;
        if(m_prevEnemyDistTime <= 0.f){
            if(m_prevDistSum > 1.f)
                m_enemyRunningAway = true;
            else
                m_enemyRunningAway = false;
            m_prevDistSum = 0.f;
            m_prevEnemyDistTime = 0.5f;
        }

        m_planner->setStateBool("IsEnemyRunningAway",m_enemyRunningAway);

        m_enemyLastPos = m_target->getSimPos();
        m_planner->setStateBool("IsEnemySeen",true);
    } else {
        m_planner->setStateBool("IsEnemyVisible",false);
        m_planner->setStateBool("IsEnemyDead",false);
        m_planner->setStateBool("IsEnemyRunningAway",false);
        m_planner->setStateFloat("EnemyDistance",1000.f);
        m_planner->setStateFloat("EnemyDistanceDiff",0.f);
        m_planner->setStateFloat("EnemyDgrDiff",0.f);
    }

    if((float)(m_enemyLastPos - getAI()->getSimPos()).length() < 3.f){
        m_planner->setStateBool("IsSpotReached",true);
        m_planner->setStateBool("IsEnemySeen",false);
    } else {
        m_planner->setStateBool("IsSpotReached",false);
    }
}

void KamilLelonekHTNActorController::executeTask(HTN::pOperator nextTask){
    ctrlrAction action = m_actions[nextTask->getName()];
    (this->*action)(nextTask->getDuration());
}

//----------actions----------
bool KamilLelonekHTNActorController::actionPatrol(float duration){
    mkVec3 new_direction = getRandomHorizontalDir();
    RayCastResult ray_result = getAI()->raycast(new_direction, 1.0f, 5.f);
    while(ray_result.hit && ray_result.collision_type == RayCastResult::Environment){
        new_direction = getRandomHorizontalDir();
        ray_result = getAI()->raycast(new_direction, 1.0f, 5.f);
    }

    size_t steps = 40;
    getAI()->startSmoothChangeDir(new_direction, steps, duration/2);
    getAI()->setSpeed(0.5f);

    return true;
}

bool KamilLelonekHTNActorController::actionRotateToEnemy(float duration){
    if(!m_target)
        return false;

    size_t steps = 40;
    mkVec3 new_direction = m_target->getSimPos() - getAI()->getSimPos();
    new_direction.normalise();
    getAI()->startSmoothChangeDir(new_direction, steps, duration);
    getAI()->setSpeed(0.f);

    return true;
}

bool KamilLelonekHTNActorController::actionAttackMelee(float duration){
    if(!m_target)
        return false;

    getAI()->hitMelee();
    getAI()->setSpeed(0.f);

    return true;
}

bool KamilLelonekHTNActorController::actionAttackFireball(float duration){
    if(!m_target)
        return false;

    
    getAI()->hitFireball(m_target->getSimPos());
    getAI()->setSpeed(0.f);
    return true;
}

bool KamilLelonekHTNActorController::actionReduceDistance(float duration){
    if(!m_target)
        return false;
    RayCastResult ray_result = getAI()->raycast(m_target->getSimPos(), 0.1f, 1.f);
    if (ray_result.hit && ray_result.collision_type == RayCastResult::Environment)
        getAI()->jump();

    mkVec3 destDir = (m_target->getSimPos()-getAI()->getSimPos()).normalisedCopy();
    size_t steps = 40;
    getAI()->startSmoothChangeDir(destDir, steps, duration);
    getAI()->setSpeed(1.f);

    return true;
}

bool KamilLelonekHTNActorController::actionRevealAttacker(float duration){
    getAI()->setSpeed(0.3f);
    size_t steps = 40;
    m_isAttacked = false;
    getAI()->startSmoothChangeDir(m_attackDir, steps, (duration+duration+duration)/4);

    return true;
}

bool KamilLelonekHTNActorController::actionAngerMode(float duration){
    if(!m_target)
        return false;

    getAI()->hitAngerMode();
    m_angerMode = true;
    return true;
}

bool KamilLelonekHTNActorController::actionExploreSpot(float duration)
{
    if(m_enemyLastPos == mkVec3::ZERO)
        return false;

    size_t steps = 40;
    mkVec3 new_direction = m_enemyLastPos - getAI()->getSimPos();
    new_direction.normalise();
    getAI()->startSmoothChangeDir(new_direction, steps, duration);
    getAI()->setSpeed(1.f);
    return true;
}

bool KamilLelonekHTNActorController::actionKeepDistance(float duration)
{
    if(!m_target)
        return false;

    size_t steps = 40;
    mkVec3 new_direction = getAI()->getSimPos() - m_target->getSimPos();
    new_direction.normalise();

    RayCastResult ray_result = getAI()->raycast(new_direction, 1.0f, 10.f);
    if(ray_result.hit && ray_result.collision_type == RayCastResult::Environment){
        return false;
    } else {
        getAI()->startSmoothChangeDir(new_direction, steps, duration/3);
        getAI()->setSpeed(1.0f);
        return true;
    }
    return false;
}

bool KamilLelonekHTNActorController::animAngerMode( float duration )
{
    if(!m_target)
        return false;

    getAI()->runAnimation("HighJump",duration);
    getAI()->setSpeed(0.f);

    return true;
}

bool KamilLelonekHTNActorController::animAttackMelee( float duration )
{
    if(!m_target)
        return false;

    getAI()->runAnimation("Attack3",duration);
    getAI()->setSpeed(0.f);

    return true;
}

bool KamilLelonekHTNActorController::animAttackFireball( float duration )
{
    if(!m_target)
        return false;

    getAI()->runAnimation("Attack1",duration);
    getAI()->setSpeed(0.f);

    return true;
}
