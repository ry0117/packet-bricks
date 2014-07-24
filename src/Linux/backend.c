/* for prints etc */
#include "pacf_log.h"
/* for string functions */
#include <string.h>
/* for io modules */
#include "io_module.h"
/* for accessing pc_info variable */
#include "main.h"
/* for function prototypes */
#include "backend.h"
/* for epoll */
#include <sys/epoll.h>
/* for socket */
#include <sys/socket.h>
/* for sockaddr_in */
#include <netinet/in.h>
/* for requests/responses */
#include "pacf_interface.h"
/* for errno */
#include <errno.h>
/* for close (2) */
#include <unistd.h>
/* for remote shell defines */
#include "lua_interpreter.h"
/* for inet_addr */
#include <arpa/inet.h>
/*---------------------------------------------------------------------*/
int
connect_to_pacf_server()
{
	TRACE_BACKEND_FUNC_START();
	int sock;
	struct sockaddr_in server;
     
	/* Create socket */
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == -1) {
		TRACE_ERR("Could not create socket: %s\n", 
			  strerror(errno));
		TRACE_BACKEND_FUNC_END();
		return -1;
	}
	TRACE_DEBUG_LOG("Socket created");
     
	server.sin_addr.s_addr = inet_addr("127.0.0.1");
	server.sin_family = AF_INET;
	server.sin_port = htons(PACF_LISTEN_PORT);
 
	/* Connect to remote server */
	if (connect(sock , (struct sockaddr *)&server , sizeof(server)) < 0) {
		TRACE_ERR("Connect failed!: %s\n", strerror(errno));
		TRACE_BACKEND_FUNC_END();
		return -1;
	}
     
	TRACE_DEBUG_LOG("Connected\n");
	TRACE_BACKEND_FUNC_END();
	return sock;
}
/*---------------------------------------------------------------------*/
void
start_listening_reqs()
{
	TRACE_BACKEND_FUNC_START();
	/* socket info about the listen sock */
	struct sockaddr_in serv; 
	int listen_fd, client_sock;
	size_t read_size, total_read;
	char client_msg[LUA_MAXINPUT];
	
	total_read = read_size = 0;
	/* zero the struct before filling the fields */
	memset(&serv, 0, sizeof(serv));
	/* set the type of connection to TCP/IP */           
	serv.sin_family = AF_INET;
	/* set the address to any interface */                
	serv.sin_addr.s_addr = htonl(INADDR_ANY); 
	/* set the server port number */    
	serv.sin_port = htons(PACF_LISTEN_PORT);
 
	listen_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (listen_fd == -1) {
		TRACE_ERR("Failed to create listening socket for pacf\n");
		TRACE_BACKEND_FUNC_END();
	}
	
	/* bind serv information to mysocket */
	if (bind(listen_fd, (struct sockaddr *)&serv, sizeof(struct sockaddr)) == -1) {
		TRACE_ERR("Failed to bind listening socket to port %d for pacf\n",
			  PACF_LISTEN_PORT);
		TRACE_BACKEND_FUNC_END();
	}
	
	/* start listening, allowing a queue of up to 1 pending connection */
	if (listen(listen_fd, LISTEN_BACKLOG) == -1) {
		TRACE_ERR("Failed to start listen on port %d (for pacf)\n",
			  PACF_LISTEN_PORT);
		TRACE_BACKEND_FUNC_END();
	}

	do {
		client_sock = accept(listen_fd, NULL, NULL);
		if (client_sock < 0) {
			TRACE_ERR("accept failed: %s\n", strerror(errno));
			TRACE_BACKEND_FUNC_END();
		}
		while (1) {
			/* Receive message from client */
			while ((read_size = recv(client_sock, 
						 &client_msg[total_read],
						 LUA_MAXINPUT - total_read, 0)) > 0) {
				total_read += read_size;
				if ((unsigned)(total_read >= LUA_MAXINPUT) || 
				    client_msg[total_read - 1] == '\0') break;
			}
			/* 
			 * if total_read == 0, 
			 * then this means that the client closed conn. 
			 */
			if (total_read == 0) break;
			/* execute the client requested cmd */
			lua_kickoff(LUA_EXE_STR, client_msg);
			total_read = 0;
		}
	} while (1);
	
	UNUSED(client_sock);
	UNUSED(total_read);
	UNUSED(client_msg);
	TRACE_BACKEND_FUNC_END();
}
/*---------------------------------------------------------------------*/
/*
 * Code self-explanatory...
 */
static void
create_listening_socket_for_eng(engine *eng)
{
	TRACE_BACKEND_FUNC_START();

	/* socket info about the listen sock */
	struct sockaddr_in serv; 
 
	/* zero the struct before filling the fields */
	memset(&serv, 0, sizeof(serv));
	/* set the type of connection to TCP/IP */           
	serv.sin_family = AF_INET;
	/* set the address to any interface */                
	serv.sin_addr.s_addr = htonl(INADDR_ANY); 
	/* set the server port number */    
	serv.sin_port = htons(PKTENGINE_LISTEN_PORT + eng->dev_fd);
 
	eng->listen_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (eng->listen_fd == -1) {
		TRACE_ERR("Failed to create listening socket for engine %s\n",
			  eng->name);
		TRACE_BACKEND_FUNC_END();
	}
	
	/* bind serv information to mysocket */
	if (bind(eng->listen_fd, (struct sockaddr *)&serv, sizeof(struct sockaddr)) == -1) {
		TRACE_ERR("Failed to bind listening socket to port %d of engine %s\n",
			  PKTENGINE_LISTEN_PORT + eng->dev_fd, eng->name);
		TRACE_BACKEND_FUNC_END();
	}
	
	/* start listening, allowing a queue of up to 1 pending connection */
	if (listen(eng->listen_fd, LISTEN_BACKLOG) == -1) {
		TRACE_ERR("Failed to start listen on port %d (engine %s)\n",
			  PKTENGINE_LISTEN_PORT + eng->dev_fd, eng->name);
		TRACE_BACKEND_FUNC_END();
	}
	eng->listen_port = PKTENGINE_LISTEN_PORT + eng->dev_fd;
	
	TRACE_BACKEND_FUNC_END();
}
/*---------------------------------------------------------------------*/
/**
 * XXX - This function needs to be revised..
 * Services incoming request from userland applications and takes
 * necessary actions. The actions can be: (i) passing packets to userland
 * apps etc.
 */
static void
process_request_backend(engine *eng, int epoll_fd)
{
	TRACE_BACKEND_FUNC_START();
	int client_sock, c, total_read;
	struct sockaddr_in client;
	int read_size;
	char client_message[2000];
	struct epoll_event ev;
	req_block *rb;
	char channelname[20];
	total_read = read_size = 0;

	/* accept connection from an incoming client */
	client_sock = accept(eng->listen_fd, (struct sockaddr *)&client, (socklen_t *)&c);
	if (client_sock < 0) {
		TRACE_LOG("accept failed: %s\n", strerror(errno));
		TRACE_BACKEND_FUNC_END();
		return;
	}
     
	/* Receive a message from client */
	while ((read_size = recv(client_sock, 
				 &client_message[total_read], 
				 sizeof(req_block) - total_read, 0)) > 0) {
		total_read += read_size;
		if ((unsigned)total_read >= sizeof(req_block)) break;
	}

	TRACE_DEBUG_LOG("Total bytes read from listening socket: %d\n", 
			total_read);
	
	/* parse new rule */
	rb = (req_block *)client_message;
	TRACE_DEBUG_LOG("Got a new rule\n");
	TRACE_DEBUG_LOG("Target: %d\n", rb->t);
	TRACE_DEBUG_LOG("TargetArgs.pid: %d\n", rb->targs.pid);
	TRACE_DEBUG_LOG("TargetArgs.proc_name: %s\n", rb->targs.proc_name);
	TRACE_FLUSH();

	//snprintf(channelname, 20, "vale%s%d:s", 
	//	 rb->targs.proc_name, rb->targs.pid);
	snprintf(channelname, 20, "%s%d", 
		 rb->targs.proc_name, rb->targs.pid);

	Rule *r = add_new_rule(eng, NULL, rb->t);

	/* create communication back channel */
	ev.data.fd = eng->iom.create_channel(eng, r, channelname/*, client_sock*/);
	ev.events = EPOLLIN | EPOLLOUT;
	if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, ev.data.fd, &ev) == -1) {
		TRACE_LOG("Engine %s failed to exe epoll_ctl for fd: %d\n",
			  eng->name, epoll_fd);
		TRACE_BACKEND_FUNC_END();
		return;
	}

	/* add client sock to the polling layer */
	ev.data.fd = client_sock;
	ev.events = EPOLLIN | EPOLLOUT;
	if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, ev.data.fd, &ev) == -1) {
		TRACE_LOG("Engine %s failed to exe epoll_ctl for fd: %d\n",
			  eng->name, epoll_fd);
		TRACE_BACKEND_FUNC_END();
		return;
	}
	
	/* continue listening */
	ev.data.fd = eng->listen_fd;
	ev.events = EPOLLIN | EPOLLOUT;
	if (epoll_ctl(epoll_fd, EPOLL_CTL_MOD, ev.data.fd, &ev) == -1) {
		TRACE_LOG("Engine %s failed to exe epoll_ctl for fd: %d\n",
			  eng->name, epoll_fd);
		TRACE_BACKEND_FUNC_END();
		return;
	}
	
	TRACE_BACKEND_FUNC_END();
	return;
}
/*---------------------------------------------------------------------*/
/**
 * Creates listening socket to serve as a conduit between userland
 * applications and the system. Starts listening on all sockets 
 * thereafter.
 */
void
initiate_backend(engine *eng)
{
	TRACE_BACKEND_FUNC_START();
	struct epoll_event ev, events[EPOLL_MAX_EVENTS];
	int epoll_fd, nfds, n;

	/* set up the epolling structure */
	epoll_fd = epoll_create(EPOLL_MAX_EVENTS);
	if (epoll_fd == -1) {
		TRACE_LOG("Engine %s failed to create an epoll fd!\n", 
			  eng->name);
		TRACE_BACKEND_FUNC_END();
		return;
	}

	/* create listening socket */
	create_listening_socket_for_eng(eng);

	/* register listening socket */
	ev.events = EPOLLIN | EPOLLOUT;
	ev.data.fd = eng->listen_fd;
	if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, eng->listen_fd, &ev) == -1) {
		TRACE_LOG("Engine %s failed to exe epoll_ctl for fd: %d\n",
			  eng->name, epoll_fd);
		TRACE_BACKEND_FUNC_END();
		return;
	}
	
	TRACE_LOG("Engine %s is listening on port %d\n", 
		  eng->name, eng->listen_port);

	/* register iom socket */
	ev.events = EPOLLIN;
	ev.data.fd = eng->dev_fd;
	
	if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, eng->dev_fd, &ev) == -1) {
		TRACE_LOG("Engine %s failed to exe epoll_ctl for fd: %d\n",
			  eng->name, epoll_fd);
		TRACE_BACKEND_FUNC_END();
		return;
	}


	/* keep on running till engine stops */
	while (eng->run == 1) {
		nfds = epoll_wait(epoll_fd, events, EPOLL_MAX_EVENTS, EPOLL_TIMEOUT);
		if (nfds == -1) {
			TRACE_ERR("epoll error (engine: %s)\n",
				  eng->name);
			TRACE_BACKEND_FUNC_END();
		}
		for (n = 0; n < nfds; n++) {
			/* process dev work */
			if (events[n].data.fd == eng->dev_fd) {
				eng->iom.callback(eng, TAILQ_FIRST(&eng->r_list));
				/* continue epolling */
				ev.data.fd = eng->dev_fd;
				ev.events = EPOLLIN;
				if (epoll_ctl(epoll_fd, EPOLL_CTL_MOD, ev.data.fd, &ev) == -1) {
					TRACE_LOG("Engine %s failed to exe epoll_ctl for fd: %d\n",
						  eng->name, epoll_fd);
					TRACE_BACKEND_FUNC_END();
					return;
				}				
			} 
			/* process app reqs */
			else if (events[n].data.fd == eng->listen_fd)
				process_request_backend(eng, epoll_fd);
		}
	}

	TRACE_BACKEND_FUNC_END();
}
/*---------------------------------------------------------------------*/