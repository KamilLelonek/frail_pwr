#include "pch.h"
#include "KamilLelonekBTActorController.h"

KamilLelonekBTActorController::KamilLelonekBTActorController( ActorAI* ai ) : IActorController(ai)
{
    m_bb				= new BT::BlackBoard();
    m_attackDir			= mkVec3::ZERO;
    m_prevEnemyDistTime = 0.f;
    m_prevEnemyDist		= 10000.f;
    m_prevDistSum		= 0.f;
    m_resetAngerMode	= false;
    m_parser			= new BT::Parser();

    m_bb -> init();
}

KamilLelonekBTActorController::~KamilLelonekBTActorController()
{
    delete m_bb;
    delete m_root;
    delete m_parser;
}

void KamilLelonekBTActorController::onCreate()
{
    getAI()  -> lookAt(mkVec3::ZERO);
    m_bb     -> setStateFloat("rngFbMax",		getAI() -> getShootingRange());
    m_bb     -> setStateFloat("rngMelee",		getAI() -> getMeleeRange());
    m_bb     -> setStateFloat("HealthAMLimit",	getAI() -> getMaxHealth()/2);
    m_bb     -> setStateFloat("dmgConeSize",	getAI() -> getMeleeConeSize());
    m_parser -> parseXmlTree(getAI() -> getBtTreePath(), getAI(), m_root);
    m_parser -> parseAliases(m_bb);
}

void KamilLelonekBTActorController::onTakeDamage( const SDamageInfo& dmg_info )
{
    m_attackDir = mkVec3::ZERO-dmg_info.dir;
}

void KamilLelonekBTActorController::onUpdate( float dt )
{
    updateWorldState(dt);
    BT::Status s = m_root -> tick(m_bb);

    if(m_parser -> isFileModified(getAI() -> getBtTreePath()) && m_root -> isTerminated()){
        m_parser -> parseXmlTree(getAI() -> getBtTreePath(), getAI(), m_root);
        m_parser -> parseAliases(m_bb);
    }
}

void KamilLelonekBTActorController::onDebugDraw()
{
    getAI() -> drawSensesInfo();
    DebugDrawer::getSingleton().drawCircle(m_enemySpot, 0.2f, 30, Ogre::ColourValue::Black, true);
}

void KamilLelonekBTActorController::onDie() { m_resetAngerMode = true; }

void KamilLelonekBTActorController::updateWorldState(float dt)
{
	mkVec3 m_healingPlace;
	if(getAI() -> isMedkitAvailable())
	{
		m_healingPlace = getAI() -> getMedkitPosition();
	}
	else
	{
		m_healingPlace = getAI() -> getPowerLakePosition();
	}
    m_bb -> setStateVec3("HealingPlace", m_healingPlace);

    if(m_resetAngerMode) {
        m_bb -> setStateBool("IsActorAM",false);
        m_resetAngerMode = false;
    }

    if(m_attackDir != mkVec3::ZERO) {
        m_bb -> setStateBool("IsEnemyAttack",true);
        m_bb -> setStateVec3("AttackDir",m_attackDir);
        m_attackDir = mkVec3::ZERO;
    }

    m_bb -> setStateFloat("ActorHealth", getAI() -> getHealth());
    m_target = getAI() -> findClosestEnemyInSight();
    if(m_target) {
        float angleDiff = (float)(getAI() -> getCharToEnemyAngle(m_target -> getSimPos()).valueDegrees());
        m_bb -> setStateFloat("EnemyDgrDiff", angleDiff);
        m_bb -> setStateBool("IsEnemySeen", true);
        m_bb -> setStateVec3("LastEnemySpot",m_target -> getSimPos());
        m_enemySpot = m_target -> getSimPos();

        m_bb -> setStateVec3("EnemyPos", m_target -> getSimPos());
        m_bb -> setStateBool("IsEnemyVisible", true);

        float enemyDistance = (float)(m_target -> getSimPos() - getAI() -> getSimPos()).length();
        m_bb -> setStateFloat("EnemyDistance", enemyDistance);

        m_prevDistSum += enemyDistance - m_prevEnemyDist;
        m_prevEnemyDist = enemyDistance;

        m_prevEnemyDistTime = m_prevEnemyDistTime - dt;
        if(m_prevEnemyDistTime <= 0.f) {
            if(m_prevDistSum > 1.f)
                m_bb -> setStateBool("IsEnemyRunningAway",true);
            else
                m_bb -> setStateBool("IsEnemyRunningAway",false);
            m_prevDistSum = 0.f;
            m_prevEnemyDistTime = 0.5f;
        }
    } else {
        m_bb -> setStateVec3("EnemyPos",		mkVec3::ZERO);
        m_bb -> setStateBool("IsEnemyVisible",	false);
        m_bb -> setStateFloat("EnemyDistance",	10000.f);
        m_bb -> setStateFloat("EnemyDgrDiff",	0.f);
    }
}