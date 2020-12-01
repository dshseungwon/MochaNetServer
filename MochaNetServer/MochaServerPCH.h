#include "MMOShared.h"

using mutex_type = std::shared_timed_mutex;
using read_only_lock  = std::shared_lock<mutex_type>;
using updatable_lock = std::unique_lock<mutex_type>;

#include "SQLAPI.h"
#include "DatabaseManager.h"
#include "ReplicationManagerTransmissionData.h"

#include "ReplicationManagerServer.h"

#include "ClientProxy.h"
#include "NetworkManagerServer.h"
#include "FirstFantasyCharacterServer.h"
#include "ArcherCharacterServer.h"
#include "StreetlightServer.h"
#include "ExplosionRPCServer.h"

#include "Server.h"


