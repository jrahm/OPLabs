#ifndef PINGTEST_HPP_
#define PINGTEST_HPP_

/*
 * Author: jrahm
 * created: 2015/02/19
 * PingTest.hpp: <description>
 */

#include <containers/BlockingQueue.hpp>
#include <proc/Process.hpp>
#include <io/binary/BufferPutter.hpp>
#include <proc/StateMachine.hpp>
#include <io/ICMPSocket.hpp>

enum PingTestPacketType {
      BEGIN_TEST
    , ACK
    , RESULTS
    , NTYPES
};

struct PingTestPacket {
    PingTestPacketType type;

    union {
        struct {
            f32_t mean_latency;
        } results;
        byte data[sizeof(results)];
    };
};

std::string toString( const PingTestPacketType& type );

inline void putObject( io::Putter& putter, const PingTestPacket& packet ) {
    putter.putByte( (byte)packet.type );
    if( packet.type == RESULTS ) {
        putter.putInt32be( floatToIntegerBytes( packet.results.mean_latency ) );   
    }
}

// int getObject<PingTestPacketType>(io::Getter&, PingTestPacketType&)
inline int getObject( io::Getter& getter, PingTestPacket& packet ) {
    packet.type = (PingTestPacketType)getter.getByte();
    u32_t u ;
    if( packet.type == RESULTS ) {
        u = getter.getInt32be();
        packet.results.mean_latency = integerBytesToFloat( u );
    }
    return 0;
}

enum PingTestStim {
    PING_RECIEVED,
    START_TEST,
    NPING_STIMS
};

enum PingState {
    IDLE,
    TEST_STARTED,
    PINGS_SENT,
    NPING_STATES,
};

const char* toString( PingTestStim stim ) {
    const char* names[] = {
        "pingRecieved", "StartTest"
    };
    return stim >= NPING_STIMS ? "Unknown" : names[stim];
}

const char* toString( PingState state ) {
    const char* names[] = {
        "Idle", "TestStarted", "PingsSent"
    };
    return state >= NPING_STATES ? "Unknown" : names[state];
}


class PingTest: public proc::Process, io::FileCollectionObserver {
public:
    PingTest();

    inline void setupStateMachine() {
        m_state_machine.setEdge(IDLE, START_TEST, &PingTest::startTest);
    }

    static inline void begin(const proc::ProcessAddress& from, proc::ProcessProxy* to) {
        PingTestPacket packet;
        packet.type = BEGIN_TEST;
        std::fill(packet.data, packet.data + sizeof(packet.data), 0);

        byte buffer[sizeof(PingTestPacket) + 1];
        io::BufferPutter putter(buffer, sizeof(buffer));
        putObject(putter, packet);

        to->sendMessage(from, buffer, putter.getSize());
    }

    /* Overridden */
    virtual void messageReceivedCallback(
        proc::ProcessAddressProxy& from_addr,
        const byte* bytes, size_t size ) OVERRIDE;

    virtual void sendAck( ProcessProxy& proxy );

    virtual void onPacketReceived(const PingTestPacket& packet);

    virtual PingState startTest() ;

    virtual inline void run() OVERRIDE {};

    void observe(int fd, int events);
    
private:
    StateMachine<PingTestStim, PingTest, PingState> m_state_machine;
    logger::LogContext& m_log;
    io::ICMPSocket* m_sock;

    containers::BlockingQueue<io::ICMPPacket> m_recieve_queue;
};

#endif /* PINGTEST_HPP_ */
