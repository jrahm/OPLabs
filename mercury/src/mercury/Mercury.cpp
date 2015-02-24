#include <mercury/Mercury.hpp>

#include <io/Inet4Address.hpp>
#include <curl/AsyncCurl.hpp>
#include <io/binary/Base64Putter.hpp>
#include <proc/ProcessManager.hpp>
#include <libjson/libjson.h>

using namespace io;
using namespace lang;
using namespace curl;
using namespace std;

namespace mercury {

byte mercury_magic_cookie[ MAGIC_COOKIE_LENGTH ] = {
    0xe2, 0x1a, 0x14, 0xc8, 0xa2, 0x35, 0x0a, 0x92,
    0xaf, 0x1a, 0xff, 0xd6, 0x35, 0x2b, 0xa4, 0xf3, 
    0x97, 0x79, 0xaf, 0xb5, 0xc1, 0x23, 0x43, 0xf0,
    0xf7, 0x14, 0x17, 0x62, 0x53, 0x4a, 0xa9, 0x7e, 
};

Mercury::SocketHandler::SocketHandler(StreamSocket* sock, Mercury* sup):
    m_sock(sock), m_sup(sup) {
}

Mercury::SocketHandler::~SocketHandler() {
    delete m_sock;
}

void Mercury::SocketHandler::observe(int fd, int events) {
    (void) events;
    /* At this point we are waiting for the magic cookie
     * to be sent */
    static logger::LogContext& log =
        logger::LogManager::instance().getLogContext("Mercury","Socket");

    log.printfln(INFO, "Witness observation in fd %d", fd);


    union {
        struct {
            byte magic_cookie[ MAGIC_COOKIE_LENGTH ];
            byte new_id[ SERVER_ID_LENGTH ];
        } str;
        byte buffer[ MAGIC_COOKIE_LENGTH + SERVER_ID_LENGTH ];
    };

    size_t bytes_read;
    bytes_read = m_sock->read( buffer, sizeof(buffer) );
    log.printHex( INFO, buffer, bytes_read );

    if( bytes_read < MAGIC_COOKIE_LENGTH + SERVER_ID_LENGTH ) {
        m_sup->m_state_machine.sendStimulus( BAD_COOKIE_RECEIVED );
        return ;
    }

    bool equal = std::equal( str.magic_cookie, str.magic_cookie + MAGIC_COOKIE_LENGTH,
                                    mercury_magic_cookie );

    if( equal ) {
        /* the magic cookie was received */
        m_sup->setId( str.new_id );
        m_sup->m_state_machine.sendStimulus(COOKIE_RECEIVED);
    } else {
        /* Something other than the magic cookie was
         * received */
        m_sup->m_state_machine.sendStimulus(BAD_COOKIE_RECEIVED);
    }
}

void Mercury::AcceptHandler::observe(int fd, int events) {
    m_sup->m_log.printfln(INFO, "Read event observed on file descriptor %d with events %02x", fd, events);
    StreamServerSocket* sock = &m_sup->m_sock;
    StreamSocket* client = sock->accept();
    m_sup->m_log.printfln(INFO, "cliend = %p", client);

    if( client ) {
        m_sup->getFileCollection().subscribe(
            FileCollection::SUBSCRIBE_READ,
            client, new SocketHandler(client, m_sup),
                lang::std_deallocator<FileCollectionObserver>());
    }
}

MercuryState Mercury::onBadRequest() {
    m_log.printfln(ERROR, "Bad request");
    return IDLE;
}

MercuryState Mercury::onGoodRequest() {
    m_log.printfln(SUCCESS, "Good request");
    m_log.printHex(INFO, &m_buffer_data[0], m_buffer_data.size());

    m_configuration.ookla_address.clear();
    m_configuration.ping_address.clear();
    m_configuration.dns_address.clear();

    parseConfigPacket(m_configuration);

    return IDLE;
}

int parse_sock_addr_vector( JSONNODE* json, vector< uptr<SocketAddress> >& vec ) {
    JSONNODE_ITERATOR i = json_begin(json);

    while ( i != json_end(i) ) {
        json_char* str = json_as_string(*i);
        SocketAddress* addr = new Inet4Address(str, 0);
        vec.push_back(addr);
    }

    return 0;
}

int parse_config_node( JSONNODE* n, ConfigPacket& pkt ) {
    JSONNODE_ITERATOR i = json_begin(n);

    while ( i != json_end(n) ) {
        json_char* name = json_name(*i);

        if( ! strcmp(name, "ookla_ips") ) {
            if ( json_type(*i) != JSON_ARRAY ) {
                return 4;
            }

            parse_sock_addr_vector( *i, pkt.ookla_address );
        }

        else if( ! strcmp(name, "ping_ips") ) {
            if ( json_type(*i) != JSON_ARRAY ) {
                return 4;
            }

            parse_sock_addr_vector( *i, pkt.ping_address );
        }

        else if( ! strcmp(name, "dns_ips") ) {
            if ( json_type(*i) != JSON_ARRAY ) {
                return 4;
            }

            parse_sock_addr_vector( *i, pkt.dns_address );
        }
    }

    return 0;
}

int Mercury::parseConfigPacket(ConfigPacket& pkt) {
    m_buffer_data.push_back(0);
    string json_str( (char*) m_buffer_data.data() );

    JSONNODE* n = json_parse(json_str.c_str());
    if( ! n || n == JSON_NULL ) {
        return 1;
    }

    JSONNODE_ITERATOR i = json_begin(n);
    char buffer[1024];
    buffer[1023] = 0;
    while ( i != json_end(n) ) {
        if ( * i == NULL || * i == JSON_NULL ) {
            return 1;
        }

        json_char* name = json_name(*i);
        json_char* value;

        if( ! strcmp(name, "status") ) {
            /* check the status */
            value = json_as_string(*i);
            strncpy(buffer, value, 1023);
            json_free(value);

            if( strcmp(buffer, "success") ) {
                /* We were not successful */
                return 2;
            }
        }

        if( ! strcmp(name, "config") ) {
            /* This is the actual config node */
            if ( json_type(*i) != JSON_NODE ) {
                /* not a node */
                return 3;
            }
            parse_config_node( *i, pkt );
        }

        ++ i;
    }

    log_config_pkt(pkt);
    return 0;
}

void Mercury::log_config_pkt(ConfigPacket& pkt) {
    std::vector< uptr<io::SocketAddress> >::iterator itr;

    m_log.printfln(DEBUG, "Got configuration:");
    m_log.printfln(DEBUG, "Ookla ips:");
    for ( itr = pkt.ookla_address.begin() ; itr != pkt.ookla_address.end() ; ++ itr )
        m_log.printfln(DEBUG, "%s", (*itr)->toString().c_str());
    m_log.printfln(DEBUG, "Ping ips:");
    for ( itr = pkt.ping_address.begin() ; itr != pkt.ping_address.end() ; ++ itr )
        m_log.printfln(DEBUG, "%s", (*itr)->toString().c_str());
    m_log.printfln(DEBUG, "Dns ips:");
    for ( itr = pkt.dns_address.begin() ; itr != pkt.dns_address.end() ; ++ itr )
        m_log.printfln(DEBUG, "%s", (*itr)->toString().c_str());
}

MercuryState Mercury::onCookieReceived() {
    m_log.printfln(INFO, "Magic cookie was received");

    MercuryCurlObserver* observer = new MercuryCurlObserver();
    observer->m_sup = this;

    Base64Putter putter;
    putter.putBytes(m_id, sizeof(m_id));
    std::string m_id_enc = putter.serialize();

    m_log.printfln(DEBUG, "Serialized id %s", m_id_enc.c_str());

    m_buffer_data.clear();
    Curl request;

    char posts[128];
    snprintf(posts, sizeof(posts), "id=%s", m_id_enc.c_str());
    request.setURL("https://128.138.202.143/api/start_test");
    request.setPostFields(posts);
    request.setFollowLocation(true);
    request.setSSLHostVerifyEnabled(false);
    request.setSSLPeerVerifyEnabled(false);

    m_log.printfln(INFO, "Sending curl request: %s", posts);
    m_asnc_curl.sendRequest(request, observer,
        lang::std_deallocator<CurlObserver>() );

    return REQUEST_MADE;
}

MercuryState Mercury::onIncorrectCookie() {
    m_log.printfln(ERROR, "Bad magic cookie found! Pause.");
    return IDLE;
}


Mercury::Mercury():
    Process("Mercury"), m_state_machine(* this, IDLE),
    m_log(logger::LogManager::instance().getLogContext("Mercury", "Internal")) {

    m_ping_test = proc::ProcessManager::instance().getProcessByName("PingTest");
    m_log.printfln(INFO, "Consttructing Mercury.");
    Inet4Address addr(INADDR_ANY, 8639);

    m_log.printfln(INFO, "Binding to address %s", addr.toString().c_str());
    m_sock.bind( addr );
    m_sock.listen();
    m_accept_handler.m_sup = this;

    setupStateMachine();

    m_log.printfln(INFO, "Subscribing for read" );
    this->getFileCollection().subscribe( FileCollection::SERVER_SOCKET, &m_sock, &m_accept_handler );
}

void Mercury::run() {
    os::Thread* th = this->newThread( m_asnc_curl );
    th->start();
    this->getFileCollection().run();
}

}
