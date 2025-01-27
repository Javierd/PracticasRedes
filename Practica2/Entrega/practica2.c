/***************************************************************************
 practica2.c
 Muestra las direciones Ethernet de la traza que se pasa como primer parametro.
 Debe complatarse con mas campos de niveles 2, 3, y 4 tal como se pida en el enunciado.
 Debe tener capacidad de dejar de analizar paquetes de acuerdo a un filtro.

 Compila: gcc -Wall -o practica2 practica2.c -lpcap, make
 Autor: Javier Delgado del Cerro, Javier López Cano
 2018 EPS-UAM
***************************************************************************/

#include <stdio.h>
#include <stdlib.h>

#include <pcap.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <linux/udp.h>
#include <linux/tcp.h>
#include <signal.h>
#include <time.h>
#include <getopt.h>
#include <inttypes.h>

/*Definicion de constantes **************************************************/
#define ETH_ALEN      6      /* Tamanio de la direccion ethernet           	*/
#define ETH_HLEN      14     /* Tamanio de la cabecera ethernet            	*/
#define ETH_TLEN      2      /* Tamanio del campo tipo ethernet            	*/
#define ETH_FRAME_MAX 1514   /* Tamanio maximo la trama ethernet (sin CRC) 	*/
#define ETH_FRAME_MIN 60     /* Tamanio minimo la trama ethernet (sin CRC) 	*/
#define ETH_DATA_MAX  (ETH_FRAME_MAX - ETH_HLEN) /* Tamano maximo y minimo de los datos de una trama ethernet*/
#define ETH_DATA_MIN  (ETH_FRAME_MIN - ETH_HLEN)
#define IP_ALEN 4			/* Tamanio de la direccion IP					*/
#define IP_TCP 6			/* Campo protocolo IP cuando es tcp 			*/
#define IP_UDP 17			/* Campo protocolo IP cuando es udp 			*/
#define OK 0
#define ERROR 1
#define PACK_READ 1
#define PACK_ERR -1
#define BREAKLOOP -2
#define NO_FILTER 0
#define NO_LIMIT -1

void analizar_paquete(u_char *user,const struct pcap_pkthdr *hdr, const uint8_t *pack);

void handleSignal(int nsignal);

pcap_t *descr = NULL;
uint64_t contador = 0;
uint8_t ipsrc_filter[IP_ALEN] = {NO_FILTER};
uint8_t ipdst_filter[IP_ALEN] = {NO_FILTER};
uint16_t sport_filter = NO_FILTER;
uint16_t dport_filter = NO_FILTER;

uint8_t ipsrc_filter_ap = 0;
uint8_t ipdst_filter_ap = 0;

void handleSignal(int nsignal)
{
	(void) nsignal; // indicamos al compilador que no nos importa que nsignal no se utilice

	printf("Control C pulsado\n");
	pcap_breakloop(descr);
}

int main(int argc, char **argv)
{
	

	char errbuf[PCAP_ERRBUF_SIZE];
	
	int long_index = 0, retorno = 0;
	char opt;
	
	(void) errbuf; //indicamos al compilador que no nos importa que errbuf no se utilice. Esta linea debe ser eliminada en la entrega final.

	if (signal(SIGINT, handleSignal) == SIG_ERR) {
		printf("Error: Fallo al capturar la senal SIGINT.\n");
		exit(ERROR);
	}

	if (argc == 1) {
		printf("Ejecucion: %s <-f traza.pcap / -i eth0> [-ipo IPO] [-ipd IPD] [-po PO] [-pd PD]\n", argv[0]);
		exit(ERROR);
	}

	static struct option options[] = {
		{"f", required_argument, 0, 'f'},
		{"i",required_argument, 0,'i'},
		{"ipo", required_argument, 0, '1'},
		{"ipd", required_argument, 0, '2'},
		{"po", required_argument, 0, '3'},
		{"pd", required_argument, 0, '4'},
		{"h", no_argument, 0, '5'},
		{0, 0, 0, 0}
	};

	/* Simple lectura por parametros por completar casos de error*/
	while ((opt = getopt_long_only(argc, argv, "f:i:1:2:3:4:5", options, &long_index)) != -1) {
		switch (opt) {
		case 'i' :
			if(descr) { /* Comprobamos que no se ha abierto ninguna otra interfaz o fichero */
				printf("Ha seleccionado más de una fuente de datos\n");
				pcap_close(descr);
				exit(ERROR);
			}
		
		
			if ((descr = pcap_open_live(optarg, ETH_FRAME_MAX, 1, 100, errbuf)) == NULL){
				printf("Error: (): Interface: %s, %s %s %d.\n", optarg,errbuf,__FILE__,__LINE__);
				exit(ERROR);
			}
			break;

		case 'f' :
			if(descr) { /* Comprobamos que no se ha abierto ninguna otra interfaz o fichero */
				printf("Ha seleccionado más de una fuente de datos\n");
				pcap_close(descr);
				exit(ERROR);
			}

			if ((descr = pcap_open_offline(optarg, errbuf)) == NULL) {
				printf("Error: pcap_open_offline(): File: %s, %s %s %d.\n", optarg, errbuf, __FILE__, __LINE__);
				exit(ERROR);
			}

			break;

		case '1' :
			if (sscanf(optarg, "%"SCNu8".%"SCNu8".%"SCNu8".%"SCNu8"", &(ipsrc_filter[0]), &(ipsrc_filter[1]), &(ipsrc_filter[2]), &(ipsrc_filter[3])) != IP_ALEN) {
				printf("Error ipo_filtro. Ejecucion: %s /ruta/captura_pcap [-ipo IPO] [-ipd IPD] [-po PO] [-pd PD]: %d\n", argv[0], argc);
				exit(ERROR);
			}

			break;

		case '2' :
			if (sscanf(optarg, "%"SCNu8".%"SCNu8".%"SCNu8".%"SCNu8"", &(ipdst_filter[0]), &(ipdst_filter[1]), &(ipdst_filter[2]), &(ipdst_filter[3])) != IP_ALEN) {
				printf("Error ipd_filtro. Ejecucion: %s /ruta/captura_pcap [-ipo IPO] [-ipd IPD] [-po PO] [-pd PD]: %d\n", argv[0], argc);
				exit(ERROR);
			}

			break;

		case '3' :
			if ((sport_filter= atoi(optarg)) == 0) {
				printf("Error po_filtro.Ejecucion: %s /ruta/captura_pcap [-ipo IPO] [-ipd IPD] [-po PO] [-pd PD]: %d\n", argv[0], argc);
				exit(ERROR);
			}

			break;

		case '4' :
			if ((dport_filter = atoi(optarg)) == 0) {
				printf("Error pd_filtro. Ejecucion: %s /ruta/captura_pcap [-ipo IPO] [-ipd IPD] [-po PO] [-pd PD]: %d\n", argv[0], argc);
				exit(ERROR);
			}

			break;

		case '5' :
			printf("Ayuda. Ejecucion: %s <-f traza.pcap / -i eth0> [-ipo IPO] [-ipd IPD] [-po PO] [-pd PD]: %d\n", argv[0], argc);
			exit(ERROR);
			break;

		case '?' :
		default:
			printf("Error. Ejecucion: %s <-f traza.pcap / -i eth0> [-ipo IPO] [-ipd IPD] [-po PO] [-pd PD]: %d\n", argv[0], argc);
			exit(ERROR);
			break;
		}
	}

	if (!descr) {
		printf("No selecciono ningún origen de paquetes.\n");
		return ERROR;
	}

	//Simple comprobacion de la correcion de la lectura de parametros
	printf("Filtro:");
	ipsrc_filter_ap = ipsrc_filter[0]!=0 && ipsrc_filter[1]!=0 && ipsrc_filter[2]!=0 && ipsrc_filter[3]!=0;
	if(ipsrc_filter_ap)
	printf("ipsrc_filter:%"PRIu8".%"PRIu8".%"PRIu8".%"PRIu8"\t", ipsrc_filter[0], ipsrc_filter[1], ipsrc_filter[2], ipsrc_filter[3]);

	ipdst_filter_ap = ipdst_filter[0]!=0 && ipdst_filter[1]!=0 && ipdst_filter[2]!=0 && ipdst_filter[3]!=0;
	if(ipdst_filter_ap)
	printf("ipdst_filter:%"PRIu8".%"PRIu8".%"PRIu8".%"PRIu8"\t", ipdst_filter[0], ipdst_filter[1], ipdst_filter[2], ipdst_filter[3]);

	if (sport_filter!= NO_FILTER) {
		printf("po_filtro=%"PRIu16"\t", sport_filter);
	}

	if (dport_filter != NO_FILTER) {
		printf("pd_filtro=%"PRIu16"\t", dport_filter);
	}

	printf("\n\n");

	retorno=pcap_loop(descr,NO_LIMIT,analizar_paquete,NULL);
	switch(retorno)	{
		case OK:
			printf("Traza leída\n");
			break;
		case PACK_ERR: 
			printf("Error leyendo paquetes\n");
			break;
		case BREAKLOOP: 
			printf("pcap_breakloop llamado\n");
			break;
	}
	printf("Se procesaron %"PRIu64" paquetes.\n\n", contador);
	pcap_close(descr);
	return OK;
}



void analizar_paquete(u_char *user,const struct pcap_pkthdr *hdr, const uint8_t *pack)
{
	(void)user;
	const uint8_t *prev;
	int i = 0;
	int protocoloETH, headerLen, totalLen, posicion, protocoloIP;
	uint16_t puertoOrigen, puertoDestino;
	uint8_t coincideIP;

	contador++;
	printf("Nuevo paquete capturado el %s", ctime((const time_t *) & (hdr->ts.tv_sec)));

	/*Campos de nivel 2*/
	printf("Direccion ETH destino: ");
	printf("%02X", pack[0]);

	for (i = 1; i < ETH_ALEN; i++) {
		printf("-%02X", pack[i]);
	}

	printf("\n");
	pack += ETH_ALEN;

	printf("Direccion ETH origen: ");
	printf("%02X", pack[0]);

	for (i = 1; i < ETH_ALEN; i++) {
		printf("-%02X", pack[i]);
	}

	printf("\n");
	
	pack+=ETH_ALEN;
	protocoloETH = ntohs(*(uint16_t *)pack);
	printf("Protocolo ETH: 0x%04X\n", protocoloETH);

 	if(protocoloETH != 2048){
 		printf("El protocolo no es IPv4. Acaba el análisis de este paquete.\n\n\n");
		return;
 	}
 	pack+= ETH_TLEN;

 	/*Campos de nivel 3*/
 	prev = pack;
 	printf("Versión IP: %d\n", pack[0] >> 4);

 	headerLen = pack[0] & 15; /*Nos quedamos solo con los 4 últimos bits*/
 	headerLen = headerLen*4; /*Pasamos a bytes*/
 	printf("Longitud de cabecera: %d\n", headerLen);
 	pack += 1 + 1; /*Saltamos el tipo de servicio*/

 	totalLen = ntohs(*(uint16_t*)pack);
 	printf("Longitud total: %d\n", totalLen);
 	pack += 2 + 2; /*Saltamos identificación*/
 	
 	posicion = ntohs((*(uint16_t *)pack)) & 8191; /*Cogemos los 16 bits, y quitamos los 3 de flags*/
 	printf("Posición/Desplazamiento: %d\n", posicion*8); /*Multiplicamos por 8 para obtener el desplazamiento real en bytes*/
	pack += 2;

	printf("Tiempo de vida: %d\n", pack[0]);
	protocoloIP = pack[1];
	printf("Protocolo nivel 4: %d\n", protocoloIP);
	pack += 2 + 2; /*Saltamos checksum*/

	coincideIP = 1;
	printf("Direccion IP origen: ");
	printf("%d", pack[0]);
	for (i = 1; i < IP_ALEN; i++) {
		printf(".%d", pack[i]);
		coincideIP = coincideIP && (pack[i] == ipsrc_filter[i]);
	}
	
	printf("\n");
	if(!coincideIP && ipsrc_filter_ap){
		printf("La IP origen no coincide con la del filtro. Acaba el análisis de este paquete.\n\n\n");
		return;
	}


	pack += 4;
	coincideIP = 1;
	printf("Direccion IP destino: ");
	printf("%d", pack[0]);
	for (i = 1; i < IP_ALEN; i++) {
		printf(".%d", pack[i]);
		coincideIP = coincideIP && (pack[i] == ipdst_filter[i]);
	}

	printf("\n");
	if(!coincideIP && ipdst_filter_ap){
		printf("La IP destino no coincide con la del filtro. Acaba el análisis de este paquete.\n\n\n");
		return;
	}

	/*Si posición != 0, no seguimos analizando*/
	if(posicion != 0){
		printf("El paquete no tiene posición 0. Acaba el análisis de este paquete.\n\n\n");
		return;
	}

	if(protocoloIP != IP_UDP &&  protocoloIP != IP_TCP){
		printf("El paquete no es UDP ni TCP. Acaba el análisis de este paquete.\n\n\n");
		return;
	}

	/*Campos de nivel 4*/
	/*Puertos de origen y destino*/
	pack = prev + headerLen;
	puertoOrigen = ntohs(*(uint16_t *)pack);
	printf("Puerto origen: %d\n", puertoOrigen);

	if(sport_filter != NO_FILTER  && puertoOrigen != sport_filter){
		printf("El puerto de origen no coincide con el del filtro. Acaba el análisis de este paquete.\n\n\n");
		return;
	}

	pack += 2;
	puertoDestino = ntohs(*(uint16_t *)pack);
	printf("Puerto destino: %d\n", puertoDestino);

	if(dport_filter != NO_FILTER  && puertoDestino != dport_filter){
		printf("El puerto de destino no coincide con el del filtro. Acaba el análisis de este paquete.\n\n\n");
		return;
	}

	pack += 2;
	if(protocoloIP == IP_UDP){
		printf("Logitud UDP: %d\n", ntohs(*(uint16_t *)pack));
	}else if(protocoloIP == IP_TCP){
		pack += 4+4+1; /*Cogemos solo el byte en el que estan las banderas*/
		printf("Bandera SYN: %d\n", (pack[0] >> 1) & 1);
		printf("Bandera FIN: %d\n", pack[0] & 1);
	}

	
	printf("\n\n");
	
}

