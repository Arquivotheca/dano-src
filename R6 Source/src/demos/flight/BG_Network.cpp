#include "BG_Erreur.h"
#include "BG_Types3D.h"
#include "BG_Memory.h"
#include "BG_Graphic.h"
#include "BG_Matrice.h"
#include "BG_PlayerInterface.h"
#include "BG_Ground.h"
#include "BG_AirCraft.h"
#include "BG_Superviseur.h"
#include "BG_Main.h"
#include "BG_Network.h"
#include <stdlib.h>
#include <stdio.h>
#include <socket.h>
#include <string.h>
#include <OS.h>
#include <Debug.h>
#include <ByteOrder.h>

static	int			nconnections;	// How many connections do we have?
static	int			NetworkMode = BG_NETWORKNULL;
static	sem_id		notify;
static	thread_id	curThread,curThread2;
static	char		KillFlag;
		sem_id		BG_semClient0,BG_semClient1;

/***********************************************************
* Descriptif :	flatten/unflatten data to be transfer through
*				the net (to canonic little-endian format).			
* Etat : Stable
* Dernière Modif :	07/10/98 (Pierre)
*					
***********************************************************/

void BG_FlattenMasterPack(BG_MasterPack *m, int32 *indexJoueur) {
	int			i;

	m->GroundKey = B_HOST_TO_LENDIAN_INT32(BG_LastGroundRandomKey);
	m->BonusKey = B_HOST_TO_LENDIAN_INT32(BG_LastBonusRandomKey);
	m->SizeGround = B_HOST_TO_LENDIAN_INT16(BG_GROUNDDH);
	m->BG_NbBonus = B_HOST_TO_LENDIAN_INT16(BG_NbBonus);
	m->PtCampA = B_HOST_TO_LENDIAN_INT16(BG_PtCampA);
	m->PtCampB = B_HOST_TO_LENDIAN_INT16(BG_PtCampB);
	for (i=0;i<2;i++) {
		if (indexJoueur[i] >= 0)
			m->LocalIndex[i] = B_HOST_TO_LENDIAN_INT16(BG_IndexPlayer[indexJoueur[i]]);
		else
			m->LocalIndex[i] = B_HOST_TO_LENDIAN_INT16(BG_IndexPlayer[BG_IndexLocalPlayer]);
	}
	BG_BlockMove((char*)BG_TableBonus, (char*)m->TableBonus, BG_NBBONUSMAX);
	for (i=0; i<BG_NBAIRCRAFTMAX; i++) {
		m->List[i].Centre.X = B_HOST_TO_LENDIAN_FLOAT(BG_ListAppareil[i].Centre.X);
		m->List[i].Centre.Y = B_HOST_TO_LENDIAN_FLOAT(BG_ListAppareil[i].Centre.Y);
		m->List[i].Centre.Z = B_HOST_TO_LENDIAN_FLOAT(BG_ListAppareil[i].Centre.Z);
		m->List[i].Rotation.X1_Y1 = B_HOST_TO_LENDIAN_FLOAT(BG_ListAppareil[i].Rotation.X1_Y1);
		m->List[i].Rotation.X1_Y2 = B_HOST_TO_LENDIAN_FLOAT(BG_ListAppareil[i].Rotation.X1_Y2);
		m->List[i].Rotation.X1_Y3 = B_HOST_TO_LENDIAN_FLOAT(BG_ListAppareil[i].Rotation.X1_Y3);
		m->List[i].Rotation.X2_Y1 = B_HOST_TO_LENDIAN_FLOAT(BG_ListAppareil[i].Rotation.X2_Y1);
		m->List[i].Rotation.X2_Y2 = B_HOST_TO_LENDIAN_FLOAT(BG_ListAppareil[i].Rotation.X2_Y2);
		m->List[i].Rotation.X2_Y3 = B_HOST_TO_LENDIAN_FLOAT(BG_ListAppareil[i].Rotation.X2_Y3);
		m->List[i].Rotation.X3_Y1 = B_HOST_TO_LENDIAN_FLOAT(BG_ListAppareil[i].Rotation.X3_Y1);
		m->List[i].Rotation.X3_Y2 = B_HOST_TO_LENDIAN_FLOAT(BG_ListAppareil[i].Rotation.X3_Y2);
		m->List[i].Rotation.X3_Y3 = B_HOST_TO_LENDIAN_FLOAT(BG_ListAppareil[i].Rotation.X3_Y3);
		m->List[i].Vitesse = B_HOST_TO_LENDIAN_FLOAT(BG_ListAppareil[i].Vitesse);
		m->List[i].Duree = B_HOST_TO_LENDIAN_FLOAT(BG_ListAppareil[i].Duree);
		m->List[i].Rayon = B_HOST_TO_LENDIAN_FLOAT(BG_ListAppareil[i].Rayon);
		m->List[i].Type = BG_ListAppareil[i].Type;
		m->List[i].Color = BG_ListAppareil[i].Color;
	}
}

void BG_UnflattenMasterPack(BG_MasterPack *m) {
	int32		GroundKey, BonusKey;
	int16		SizeGround;
	int32		i;

	GroundKey = B_LENDIAN_TO_HOST_INT32(m->GroundKey);
	BonusKey = B_LENDIAN_TO_HOST_INT32(m->BonusKey);
	SizeGround = B_LENDIAN_TO_HOST_INT16(m->SizeGround);
	if ((BG_GROUNDDH != SizeGround) || (GroundKey != BG_LastGroundRandomKey)) {
		BG_NewGround(SizeGround, GroundKey);
		BG_ResetGround(BonusKey);
	}
	else if (BonusKey != BG_LastBonusRandomKey)
		BG_ResetGround(BonusKey);
	BG_NbBonus = B_LENDIAN_TO_HOST_INT16(m->BG_NbBonus);
	
	BG_PtCampA = B_LENDIAN_TO_HOST_INT16(m->PtCampA);
	BG_PtCampB = B_LENDIAN_TO_HOST_INT16(m->PtCampB);
	BG_IndexLocalPlayer = B_LENDIAN_TO_HOST_INT16(m->LocalIndex[0]);
	BG_IndexLocalPlayer2 = B_LENDIAN_TO_HOST_INT16(m->LocalIndex[1]);
	BG_BlockMove((char*)m->TableBonus, (char*)BG_TableBonus, BG_NBBONUSMAX);
	for (i=0; i<BG_NBAIRCRAFTMAX; i++)
		if (BG_ListAppareil[i].Type >= 0)
			BG_RemoveObject(BG_ListAppareil+i);
	for (i=0; i<BG_NBAIRCRAFTMAX; i++) {
		BG_ListAppareil[i].Centre.X = B_LENDIAN_TO_HOST_FLOAT(m->List[i].Centre.X);
		BG_ListAppareil[i].Centre.Y = B_LENDIAN_TO_HOST_FLOAT(m->List[i].Centre.Y);
		BG_ListAppareil[i].Centre.Z = B_LENDIAN_TO_HOST_FLOAT(m->List[i].Centre.Z);
		BG_ListAppareil[i].Rotation.X1_Y1 = B_LENDIAN_TO_HOST_FLOAT(m->List[i].Rotation.X1_Y1);
		BG_ListAppareil[i].Rotation.X1_Y2 = B_LENDIAN_TO_HOST_FLOAT(m->List[i].Rotation.X1_Y2);
		BG_ListAppareil[i].Rotation.X1_Y3 = B_LENDIAN_TO_HOST_FLOAT(m->List[i].Rotation.X1_Y3);
		BG_ListAppareil[i].Rotation.X2_Y1 = B_LENDIAN_TO_HOST_FLOAT(m->List[i].Rotation.X2_Y1);
		BG_ListAppareil[i].Rotation.X2_Y2 = B_LENDIAN_TO_HOST_FLOAT(m->List[i].Rotation.X2_Y2);
		BG_ListAppareil[i].Rotation.X2_Y3 = B_LENDIAN_TO_HOST_FLOAT(m->List[i].Rotation.X2_Y3);
		BG_ListAppareil[i].Rotation.X3_Y1 = B_LENDIAN_TO_HOST_FLOAT(m->List[i].Rotation.X3_Y1);
		BG_ListAppareil[i].Rotation.X3_Y2 = B_LENDIAN_TO_HOST_FLOAT(m->List[i].Rotation.X3_Y2);
		BG_ListAppareil[i].Rotation.X3_Y3 = B_LENDIAN_TO_HOST_FLOAT(m->List[i].Rotation.X3_Y3);
		BG_ListAppareil[i].Vitesse = B_LENDIAN_TO_HOST_FLOAT(m->List[i].Vitesse);
		BG_ListAppareil[i].Duree = B_LENDIAN_TO_HOST_FLOAT(m->List[i].Duree);
		BG_ListAppareil[i].Rayon = B_LENDIAN_TO_HOST_FLOAT(m->List[i].Rayon);
		BG_ListAppareil[i].Type = m->List[i].Type;
		BG_ListAppareil[i].Color = m->List[i].Color;
	}
	for (i=0; i<BG_NBAIRCRAFTMAX; i++)
		if (BG_ListAppareil[i].Type >= 0)
			BG_AddObject(BG_ListAppareil+i);
}

void BG_SwapVPaddle(BG_VPaddle *from, BG_VPaddle *to) {
	to->deltaH = B_HOST_TO_BENDIAN_INT16(from->deltaH);
	to->deltaV = B_HOST_TO_BENDIAN_INT16(from->deltaV);
	to->button[0] = from->button[0];
	to->button[1] = from->button[1];
	to->button[2] = from->button[2];
	to->button[3] = from->button[3];
}

/*
void BG_SwapSockAddrIn(struct sockaddr_in *si) {
	si->sin_family = B_HOST_TO_LENDIAN_INT16(si->sin_family);
	si->sin_port = B_HOST_TO_LENDIAN_INT16(si->sin_port);
	si->sin_addr = B_HOST_TO_LENDIAN_INT32(si->sin_addr);
}
*/

/***********************************************************
* Descriptif :	thread to handle an individual connection
*				
* Etat : En cours
* Dernière Modif :	07/09/94 (Pierre)
*					
***********************************************************/
long BG_TcpHandler(void *v) {
	int				i, s, len, tlen;
	int32			indexJoueur[2];
	char			buf[BUFSIZE];
	char			*tbuf;
	BG_SlavePack	*In;
	BG_MasterPack	*Out;
	
	s = (int)v;
	acquire_sem(notify);
// init players ref
	indexJoueur[0] = -1;
	indexJoueur[1] = -1;
	while (true) {
	// send the state of the world
		Out = (BG_MasterPack*)buf;
		acquire_sem(BG_semJoueur);
		BG_FlattenMasterPack(Out, indexJoueur);
		release_sem(BG_semJoueur);
		if ((len = send(s, buf, sizeof(BG_MasterPack), 0)) <= 0) break;
	// recupere un paquet de l'esclave
		tlen = sizeof(BG_SlavePack);
		tbuf = buf;
		while (tlen > 0) {
			len = recv(s, tbuf, tlen, 0);
			if (len < 0) break;
			tlen -= len;
			tbuf += len;
		}
		if (tlen > 0)
			break;
		In = (BG_SlavePack*)buf;
		if (In->KillFlag) break;
		if (!BG_GameEnable) break;
		if (indexJoueur[0] == -1) {
			indexJoueur[0] = BG_ReservePlayer(In->YellowMode[0]);
			if (indexJoueur[0] == BG_NBPLAYERMAX) break;
		}
		if ((indexJoueur[1] == -1) && (In->SndPlrEnable)) {
			indexJoueur[1] = BG_ReservePlayer(In->YellowMode[1]);
			if (indexJoueur[1] == BG_NBPLAYERMAX) break;
		}
		acquire_sem(BG_semJoueur);
		for (i=0;i<2;i++)
			if (indexJoueur[i] >= 0)
				BG_SwapVPaddle(In->Pad+i, BG_ListPaddle+indexJoueur[i]);
		release_sem(BG_semJoueur);
	}
	nconnections--;
	closesocket(s);
	for (i=0;i<2;i++)
		if (indexJoueur[i] >= 0)
			BG_ReleasePlayer(indexJoueur[i]);
	return 0L;
}

/***********************************************************
* Descriptif :	thread to wait for new connections, and spawn
*				off new threads for them
* Etat : En cours
* Dernière Modif :	07/09/94 (Pierre)
*					
***********************************************************/
long BG_ServerThread(void *v) {
	int					s, s2, fromlen;
	struct sockaddr_in	from;

	s = (int)v;
	listen(s, 5);
	while (true) {
		fromlen = sizeof(from);
		printf("Waiting for connection...\n");
		s2 = accept(s, (struct sockaddr *)&from, &fromlen);
		if (s2 < 0) {
			printf("accept failed\n");
			closesocket(s);
			return 0L;
		}
		printf("Got a new connection (%d)\n", s2);
		nconnections++;
		release_sem_etc(notify, nconnections, 0);
		resume_thread(spawn_thread(BG_TcpHandler,"", B_REAL_TIME_PRIORITY,(void *)s2));
	}
}

/***********************************************************
* Descriptif :	start the tcp listener thread, and return its
*				port number to the caller
* Etat : En cours
* Dernière Modif :	07/09/94 (Pierre)
*					
***********************************************************/
int BG_StartTcpListener(void) {
	int 				sinlen,s;
	struct sockaddr_in	sin;

	s = socket(AF_INET, SOCK_STREAM, 0);
	sin.sin_addr.s_addr = BOUND_ADDR;
	sin.sin_family = AF_INET;
	sin.sin_port = 0;

	if (bind(s, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
		fprintf(stderr, "cannot bind udp socket\n");
		return (0);
	}
	sinlen = sizeof(sin);
	if (getsockname(s, (struct sockaddr *)&sin, &sinlen) < 0) {
		fprintf(stderr, "cannot get sock name\n");
		return (0);
	}
	printf("Now serving clients...\n");	
	curThread2 = spawn_thread(BG_ServerThread,"",B_NORMAL_PRIORITY,(void *)s);
	resume_thread(curThread2);
	return (sin.sin_port);
}

/***********************************************************
* Descriptif :	Init Server
*				
* Etat : En cours
* Dernière Modif :	21/11/95 (Pierre)
*					
***********************************************************/
long BG_Server(void) {
	int 				fromlen, s, len, tlen;
	char				buf[sizeof(short)];
	char				*tbuf;
	short				myport;
	struct sockaddr_in	sin,from;

	notify = create_sem(0, "notify");
	myport = BG_StartTcpListener();
// Create a UDP port to listen on so that clients can find us
	s = socket(AF_INET, SOCK_DGRAM, 0);
// Bind it to the well know port
	sin.sin_addr.s_addr = BOUND_ADDR;
	sin.sin_family = AF_INET;
	sin.sin_port = htons(BG_WELL_KNOWN_PORT);
	if (bind(s, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
		fprintf(stderr, "cannot bind udp socket\n");
		return 0L;
	}
// Then wait for requests
	while (true) {
		fromlen = sizeof(from);
		tlen = sizeof(buf);
		tbuf = buf;
		while (tlen > 0) {
			len = recvfrom(s, tbuf, tlen, 0,(struct sockaddr*)&from, &fromlen);
			if (len < 0) {
				fprintf(stderr, "fatal error trying to receive UDP\n");
				return 0L;
			}
			tbuf += len;
			tlen -= len;
		}
	// Send them back our TCP port number
		sendto(s, (char *)&myport, sizeof(myport), 0,(struct sockaddr *)&from, sizeof(from));
	}
	return 0L;
}

/***********************************************************
* Descriptif :	Find a server and return its address
*				
* Etat : En cours
* Dernière Modif :	07/09/94 (Pierre)
*					
***********************************************************/
int BG_FindServer(struct sockaddr_in *sinp) {
	int					s, n, len, tlen, sinlen;
	char				*tbuf;
	short				port;
	struct fd_set		fds;
	struct timeval		tv;
	struct sockaddr_in	sin,to;

// Create udp socket and bind an address to it
	s = socket(AF_INET, SOCK_DGRAM, 0);
	sin.sin_addr.s_addr = BOUND_ADDR;
	sin.sin_family = AF_INET;
	sin.sin_port = 0;
	if (bind(s, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
		fprintf(stderr, "cannot bind tcp socket\n");
		closesocket(s);
		return (0);
	}
// Create a broadcast address
	to.sin_addr.s_addr = BROADCAST_ADDR;
	to.sin_port = htons(BG_WELL_KNOWN_PORT);
	to.sin_family = AF_INET;
// Send out the broadcast request 
	port = 0;
	printf("Sending broadcast packet\n");
	sendto(s, (char *)&port, sizeof(port), 0,
		   (struct sockaddr *)&to, sizeof(to));
// Then, wait for a reply with a given timeout
	// Timeout set to 5 seconds
	tv.tv_sec = 5;
	tv.tv_usec = 0;
	FD_ZERO(&fds);
	FD_SET(s, &fds);
	printf("waiting for reply\n");
	n = select(s + 1, &fds, NULL, NULL, &tv);
	if (n < 0) {
		printf("select failed\n");
		closesocket(s);
		return (0);
	}
	if (n == 0) {
		printf("select timed out\n");
		closesocket(s);
		return (0);
	}
// We have data waiting, lets get it
	sinlen = sizeof(sin);
	tlen = sizeof(port);
	tbuf = (char*)&port;
	while (tlen > 0) {
		len = recvfrom(s, tbuf, tlen, 0, (struct sockaddr *)&sin, &sinlen);
		if (len < 0) {
			printf("Invalid data\n");
			return (0);
		}
		tlen -= len;
		tbuf += len;
	}
	closesocket(s);
// valid data, return it.
	printf("We found a server\n");
	*sinp = sin;
	sinp->sin_port = port;
	return (1);
}

/***********************************************************
* Descriptif :	Client
*				
* Etat : En cours
* Dernière Modif :	07/09/94 (Pierre)
*					
***********************************************************/
long BG_Client(void)
	{
	int					i,s,len,tlen;
	char				buf[sizeof(BG_MasterPack)];
	char				*tbuf;
	char				buf2[sizeof(BG_SlavePack)];
	BG_SlavePack		*Out;
	BG_MasterPack		*In;
	struct sockaddr_in	sin;

	if (!BG_FindServer(&sin))
		{
		printf("No server found\n");
		return 0L;
		}
	s = socket(AF_INET, SOCK_STREAM, 0);
	printf("Connecting to server\n");
	if (connect(s, (struct sockaddr *)&sin, sizeof(sin)) < 0)
		{
		printf("cannot connect to server\n");
		return 0L;
		}
	printf("Connected to server\n");
	while (TRUE)
		{
	// recoit le paquet du maitre
		tlen = sizeof(buf);
		tbuf = buf;
		while (tlen > 0) {
			len = recv(s, tbuf, tlen, 0);
			if (len < 0) break;
			tlen -= len;
			tbuf += len;
		}
		if (tlen > 0)
			break;
	// envoie son paquet
		Out = (BG_SlavePack*)buf2;
		for (i=0;i<2;i++)
			Out->YellowMode[i] = BG_MaWindow[i]->YellowEnable;
		Out->KillFlag = KillFlag;
		Out->SndPlrEnable = BG_SndPlrEnable;
		acquire_sem(BG_semJoueur);
		if (BG_IndexLocalPlayer >= 0)
			BG_SwapVPaddle(BG_ListPaddle+BG_IndexLocalPlayer, Out->Pad+0);
		if (BG_IndexLocalPlayer2 >= 0)
			BG_SwapVPaddle(BG_ListPaddle+BG_IndexLocalPlayer2, Out->Pad+1);
		release_sem(BG_semJoueur);
		if (send(s, buf2, sizeof(BG_SlavePack), 0) <= 0) break;
		if (KillFlag) break;
	// decode la nouvelle image
		In = (BG_MasterPack*)buf;
		acquire_sem(BG_semClient0);
		BG_UnflattenMasterPack(In);
		release_sem(BG_semClient1);
		}
	printf("server has died\n");
	return 0L;
	}

/***********************************************************
* Descriptif :	Selecteur de mode maitre/esclave
*				
* Etat : En cours
* Dernière Modif :	07/09/94 (Pierre)
*					
***********************************************************/
void BG_SelectNetworkMode(int mode)
	{
	long		result;
	
// rien si pas de changement
	if (mode == NetworkMode)
		return;
// desactive l'ancien mode
	switch (NetworkMode)
		{
		case BG_NETWORKMASTER :
//			suspend_thread(curThread2);
//			suspend_thread(curThread);
			kill_thread(curThread2);
			kill_thread(curThread);
//			resume_thread(curThread2);
//			resume_thread(curThread);
			break;
		case BG_NETWORKSLAVE :
			suspend_thread(curThread);
			KillFlag = TRUE;
			release_sem(BG_semClient0);
			resume_thread(curThread);
			wait_for_thread(curThread,&result);
			delete_sem(BG_semClient0);
			delete_sem(BG_semClient1);
			break;
		}
	BG_ResetSuperviseur();
// change de mode
	NetworkMode = mode;
// active le nouveau mode
	switch (mode)
		{
		case BG_NETWORKMASTER :
			curThread = spawn_thread((thread_entry)BG_Server,"",B_NORMAL_PRIORITY,0L);
			resume_thread(curThread);
			break;
		case BG_NETWORKSLAVE :
			BG_semClient0 = create_sem(1,"SClient0");
			BG_semClient1 = create_sem(0,"SClient1");
			curThread = spawn_thread((thread_entry)BG_Client,"",B_URGENT_DISPLAY_PRIORITY,0L);
			KillFlag = FALSE;
			resume_thread(curThread);
			break;
		}
	}

