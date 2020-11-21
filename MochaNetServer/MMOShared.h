#pragma once

#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <netdb.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

typedef int SOCKET;
const int NO_ERROR = 0;
const int INVALID_SOCKET = -1;
const int WSAECONNRESET = ECONNRESET;
const int WSAEWOULDBLOCK = EAGAIN;
const int SOCKET_ERROR = -1;


#include "memory"

#include "vector"
#include "unordered_map"
#include "string"
#include "list"
#include "queue"
#include "deque"
#include "unordered_set"
#include "cassert"

using std::shared_ptr;
using std::unique_ptr;
using std::vector;
using std::queue;
using std::list;
using std::deque;
using std::unordered_map;
using std::string;
using std::unordered_set;

class GameObject;

#include "MMOMath.h"
#include "MMOByteSwap.h"
#include "MemoryBitStream.h"

#include "StringUtils.h"
#include "SocketAddress.h"
#include "SocketAddressFactory.h"
#include "UDPSocket.h"
#include "TCPSocket.h"
#include "SocketUtil.h"

#include "Timing.h"
#include "WeightedTimeMovingAverage.h"

#include "MochaObject.h"
#include "NetworkManager.h"


#include "MMOPeer.h"
#include "ReplicationCommand.h"
#include "GameObjectRegistry.h"

#include "MMOWorld.h"

#include "MMOInputState.h"
#include "Move.h"
#include "MoveList.h"
