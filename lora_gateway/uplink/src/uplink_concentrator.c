// DEPENDENCIES

/* fix an issue between POSIX and C99 */
#if __STDC_VERSION__ >= 199901L
	#define _XOPEN_SOURCE 600
#else
	#define _XOPEN_SOURCE 500
#endif

#include <stdint.h>		/* C99 types */
#include <stdbool.h>	/* bool type */
#include <stdio.h>		/* printf fprintf sprintf fopen fputs */

#include <string.h>		/* memset */
#include <signal.h>		/* sigaction */
#include <time.h>		/* time clock_gettime strftime gmtime clock_nanosleep*/
#include <unistd.h>		/* getopt access */
#include <stdlib.h>		/* atoi */
#include <math.h>

#include "parson.h"
#include "loragw_hal.h"

// CONSTANTS

#define ROUTER_ID "0200000000EEFFC0"
#define DEVICE_ID "0123456789ABCDEF"

#define JOIN_RESPONSE_FREQ 869525000 // 869.525 MHz 
#define JOIN_RESPONSE_DELAY 2000000 // 6 seconds in us
#define JOIN_RF_CHAIN 0
#define JOIN_RESPONSE_POWER 14

#define JOIN_REQ_MSG 1
#define TEST_MSG 2
#define END_TEST_MSG 3
#define ALL_TESTS_ENDED_MSG 4
#define INVALID_MSG -1

#define MAX_MSGS_PER_SETTING 100

// PRIVATE MACROS

#define ARRAY_SIZE(a)	(sizeof(a) / sizeof((a)[0]))
#define MSG(args...)	fprintf(stderr, "uplink_concentrator: " args) /* message that is destined to the user */

// PRIVATE VARIABLES (GLOBAL)

/* signal handling variables */
struct sigaction sigact; /* SIGQUIT&SIGINT&SIGTERM signal handling */
static int exit_sig = 0; /* 1 -> application terminates cleanly (shut down hardware, close open files, etc) */
static int quit_sig = 0; /* 1 -> application terminates without shutting down the hardware */

/* configuration variables needed by the application  */
uint64_t lgwm = 0; /* LoRa gateway MAC address */
char lgwm_str[17];

/* clock and log file management */
time_t now_time;
time_t log_start_time;
static int size = 0;

char *result_file_name = "results.csv";
FILE* result_file = NULL;

float snr[MAX_MSGS_PER_SETTING];

// PRIVATE FUNCTIONS DECLARATION

static void sig_handler(int sigio);
void usage(void);
int compare_id(struct lgw_pkt_rx_s*);
void configure_gateway(void);
int parse_SX1301_configuration(const char * conf_file);
int parse_gateway_configuration(const char * conf_file);

// PRIVATE FUNCTIONS DEFINITION

void configure_gateway(void) {
	/* configuration file related */
	const char global_conf_fname[] = "global_conf.json"; /* contain global (typ. network-wide) configuration */
	const char local_conf_fname[] = "local_conf.json"; /* contain node specific configuration, overwrite global parameters for parameters that are defined in both */
	const char debug_conf_fname[] = "debug_conf.json"; /* if present, all other configuration files are ignored */

	/* configuration files management */
	if (access(debug_conf_fname, R_OK) == 0) {
	/* if there is a debug conf, parse only the debug conf */
		MSG("INFO: found debug configuration file %s, other configuration files will be ignored\n", debug_conf_fname);
		parse_SX1301_configuration(debug_conf_fname);
		parse_gateway_configuration(debug_conf_fname);
	} else if (access(global_conf_fname, R_OK) == 0) {
	/* if there is a global conf, parse it and then try to parse local conf  */
		MSG("INFO: found global configuration file %s, trying to parse it\n", global_conf_fname);
		parse_SX1301_configuration(global_conf_fname);
		parse_gateway_configuration(global_conf_fname);
		if (access(local_conf_fname, R_OK) == 0) {
			MSG("INFO: found local configuration file %s, trying to parse it\n", local_conf_fname);
			parse_SX1301_configuration(local_conf_fname);
			parse_gateway_configuration(local_conf_fname);
		}
	} else if (access(local_conf_fname, R_OK) == 0) {
	/* if there is only a local conf, parse it and that's all */
		MSG("INFO: found local configuration file %s, trying to parse it\n", local_conf_fname);
		parse_SX1301_configuration(local_conf_fname);
		parse_gateway_configuration(local_conf_fname);
	} else {
		MSG("ERROR: failed to find any configuration file named %s, %s or %s\n", global_conf_fname, local_conf_fname, debug_conf_fname);
		exit(EXIT_FAILURE);
	}
	
}

int parse_SX1301_configuration(const char * conf_file) {
	int i;
	const char conf_obj[] = "SX1301_conf";
	char param_name[32]; /* used to generate variable parameter names */
	const char *str; /* used to store string value from JSON object */
	struct lgw_conf_board_s boardconf;
	struct lgw_conf_rxrf_s rfconf;
	struct lgw_conf_rxif_s ifconf;
	JSON_Value *root_val;
	JSON_Object *root = NULL;
	JSON_Object *conf = NULL;
	JSON_Value *val;
	uint32_t sf, bw;
	
	/* try to parse JSON */
	root_val = json_parse_file_with_comments(conf_file);
	root = json_value_get_object(root_val);
	if (root == NULL) {
		MSG("ERROR: %s id not a valid JSON file\n", conf_file);
		exit(EXIT_FAILURE);
	}
	conf = json_object_get_object(root, conf_obj);
	if (conf == NULL) {
		MSG("INFO: %s does not contain a JSON object named %s\n", conf_file, conf_obj);
		return -1;
	} else {
		MSG("INFO: %s does contain a JSON object named %s, parsing SX1301 parameters\n", conf_file, conf_obj);
	}

	/* set board configuration */
	memset(&boardconf, 0, sizeof boardconf); /* initialize configuration structure */
	val = json_object_get_value(conf, "lorawan_public"); /* fetch value (if possible) */
	if (json_value_get_type(val) == JSONBoolean) {
		boardconf.lorawan_public = (bool)json_value_get_boolean(val);
	} else {
		MSG("WARNING: Data type for lorawan_public seems wrong, please check\n");
		boardconf.lorawan_public = false;
	}
	val = json_object_get_value(conf, "clksrc"); /* fetch value (if possible) */
	if (json_value_get_type(val) == JSONNumber) {
		boardconf.clksrc = (uint8_t)json_value_get_number(val);
	} else {
		MSG("WARNING: Data type for clksrc seems wrong, please check\n");
		boardconf.clksrc = 0;
	}
	MSG("INFO: lorawan_public %d, clksrc %d\n", boardconf.lorawan_public, boardconf.clksrc);
	/* all parameters parsed, submitting configuration to the HAL */
        if (lgw_board_setconf(boardconf) != LGW_HAL_SUCCESS) {
                MSG("WARNING: Failed to configure board\n");
	}

	/* set configuration for RF chains */
	for (i = 0; i < LGW_RF_CHAIN_NB; ++i) {
		memset(&rfconf, 0, sizeof(rfconf)); /* initialize configuration structure */
		sprintf(param_name, "radio_%i", i); /* compose parameter path inside JSON structure */
		val = json_object_get_value(conf, param_name); /* fetch value (if possible) */
		if (json_value_get_type(val) != JSONObject) {
			MSG("INFO: no configuration for radio %i\n", i);
			continue;
		}
		/* there is an object to configure that radio, let's parse it */
		sprintf(param_name, "radio_%i.enable", i);
		val = json_object_dotget_value(conf, param_name);
		if (json_value_get_type(val) == JSONBoolean) {
			rfconf.enable = (bool)json_value_get_boolean(val);
		} else {
			rfconf.enable = false;
		}
		if (rfconf.enable == false) { /* radio disabled, nothing else to parse */
			MSG("INFO: radio %i disabled\n", i);
		} else  { /* radio enabled, will parse the other parameters */
			snprintf(param_name, sizeof param_name, "radio_%i.freq", i);
			rfconf.freq_hz = (uint32_t)json_object_dotget_number(conf, param_name);
			snprintf(param_name, sizeof param_name, "radio_%i.rssi_offset", i);
			rfconf.rssi_offset = (float)json_object_dotget_number(conf, param_name);
			snprintf(param_name, sizeof param_name, "radio_%i.type", i);
			str = json_object_dotget_string(conf, param_name);
			if (!strncmp(str, "SX1255", 6)) {
				rfconf.type = LGW_RADIO_TYPE_SX1255;
			} else if (!strncmp(str, "SX1257", 6)) {
				rfconf.type = LGW_RADIO_TYPE_SX1257;
			} else {
				MSG("WARNING: invalid radio type: %s (should be SX1255 or SX1257)\n", str);
			}
			snprintf(param_name, sizeof param_name, "radio_%i.tx_enable", i);
			val = json_object_dotget_value(conf, param_name);
			if (json_value_get_type(val) == JSONBoolean) {
				rfconf.tx_enable = (bool)json_value_get_boolean(val);
			} else {
				rfconf.tx_enable = false;
			}
			MSG("INFO: radio %i enabled (type %s), center frequency %u, RSSI offset %f, tx enabled %d\n", i, str, rfconf.freq_hz, rfconf.rssi_offset, rfconf.tx_enable);
		}
		/* all parameters parsed, submitting configuration to the HAL */
		if (lgw_rxrf_setconf(i, rfconf) != LGW_HAL_SUCCESS) {
			MSG("WARNING: invalid configuration for radio %i\n", i);
		}
	}
	
	/* set configuration for LoRa multi-SF channels (bandwidth cannot be set) */
	for (i = 0; i < LGW_MULTI_NB; ++i) {
		memset(&ifconf, 0, sizeof(ifconf)); /* initialize configuration structure */
		sprintf(param_name, "chan_multiSF_%i", i); /* compose parameter path inside JSON structure */
		val = json_object_get_value(conf, param_name); /* fetch value (if possible) */
		if (json_value_get_type(val) != JSONObject) {
			MSG("INFO: no configuration for LoRa multi-SF channel %i\n", i);
			continue;
		}
		/* there is an object to configure that LoRa multi-SF channel, let's parse it */
		sprintf(param_name, "chan_multiSF_%i.enable", i);
		val = json_object_dotget_value(conf, param_name);
		if (json_value_get_type(val) == JSONBoolean) {
			ifconf.enable = (bool)json_value_get_boolean(val);
		} else {
			ifconf.enable = false;
		}
		if (ifconf.enable == false) { /* LoRa multi-SF channel disabled, nothing else to parse */
			MSG("INFO: LoRa multi-SF channel %i disabled\n", i);
		} else  { /* LoRa multi-SF channel enabled, will parse the other parameters */
			sprintf(param_name, "chan_multiSF_%i.radio", i);
			ifconf.rf_chain = (uint32_t)json_object_dotget_number(conf, param_name);
			sprintf(param_name, "chan_multiSF_%i.if", i);
			ifconf.freq_hz = (int32_t)json_object_dotget_number(conf, param_name);
			// TODO: handle individual SF enabling and disabling (spread_factor)
			MSG("INFO: LoRa multi-SF channel %i enabled, radio %i selected, IF %i Hz, 125 kHz bandwidth, SF 7 to 12\n", i, ifconf.rf_chain, ifconf.freq_hz);
		}
		/* all parameters parsed, submitting configuration to the HAL */
		if (lgw_rxif_setconf(i, ifconf) != LGW_HAL_SUCCESS) {
			MSG("WARNING: invalid configuration for LoRa multi-SF channel %i\n", i);
		}
	}
	
	/* set configuration for LoRa standard channel */
	memset(&ifconf, 0, sizeof(ifconf)); /* initialize configuration structure */
	val = json_object_get_value(conf, "chan_Lora_std"); /* fetch value (if possible) */
	if (json_value_get_type(val) != JSONObject) {
		MSG("INFO: no configuration for LoRa standard channel\n");
	} else {
		val = json_object_dotget_value(conf, "chan_Lora_std.enable");
		if (json_value_get_type(val) == JSONBoolean) {
			ifconf.enable = (bool)json_value_get_boolean(val);
		} else {
			ifconf.enable = false;
		}
		if (ifconf.enable == false) {
			MSG("INFO: LoRa standard channel %i disabled\n", i);
		} else  {
			ifconf.rf_chain = (uint32_t)json_object_dotget_number(conf, "chan_Lora_std.radio");
			ifconf.freq_hz = (int32_t)json_object_dotget_number(conf, "chan_Lora_std.if");
			bw = (uint32_t)json_object_dotget_number(conf, "chan_Lora_std.bandwidth");
			switch(bw) {
				case 500000: ifconf.bandwidth = BW_500KHZ; break;
				case 250000: ifconf.bandwidth = BW_250KHZ; break;
				case 125000: ifconf.bandwidth = BW_125KHZ; break;
				default: ifconf.bandwidth = BW_UNDEFINED;
			}
			sf = (uint32_t)json_object_dotget_number(conf, "chan_Lora_std.spread_factor");
			switch(sf) {
				case  7: ifconf.datarate = DR_LORA_SF7;  break;
				case  8: ifconf.datarate = DR_LORA_SF8;  break;
				case  9: ifconf.datarate = DR_LORA_SF9;  break;
				case 10: ifconf.datarate = DR_LORA_SF10; break;
				case 11: ifconf.datarate = DR_LORA_SF11; break;
				case 12: ifconf.datarate = DR_LORA_SF12; break;
				default: ifconf.datarate = DR_UNDEFINED;
			}
			MSG("INFO: LoRa standard channel enabled, radio %i selected, IF %i Hz, %u Hz bandwidth, SF %u\n", ifconf.rf_chain, ifconf.freq_hz, bw, sf);
		}
		if (lgw_rxif_setconf(8, ifconf) != LGW_HAL_SUCCESS) {
			MSG("WARNING: invalid configuration for LoRa standard channel\n");
		}
	}
	
	/* set configuration for FSK channel */
	memset(&ifconf, 0, sizeof(ifconf)); /* initialize configuration structure */
	val = json_object_get_value(conf, "chan_FSK"); /* fetch value (if possible) */
	if (json_value_get_type(val) != JSONObject) {
		MSG("INFO: no configuration for FSK channel\n");
	} else {
		val = json_object_dotget_value(conf, "chan_FSK.enable");
		if (json_value_get_type(val) == JSONBoolean) {
			ifconf.enable = (bool)json_value_get_boolean(val);
		} else {
			ifconf.enable = false;
		}
		if (ifconf.enable == false) {
			MSG("INFO: FSK channel %i disabled\n", i);
		} else  {
			ifconf.rf_chain = (uint32_t)json_object_dotget_number(conf, "chan_FSK.radio");
			ifconf.freq_hz = (int32_t)json_object_dotget_number(conf, "chan_FSK.if");
			bw = (uint32_t)json_object_dotget_number(conf, "chan_FSK.bandwidth");
			if      (bw <= 7800)   ifconf.bandwidth = BW_7K8HZ;
			else if (bw <= 15600)  ifconf.bandwidth = BW_15K6HZ;
			else if (bw <= 31200)  ifconf.bandwidth = BW_31K2HZ;
			else if (bw <= 62500)  ifconf.bandwidth = BW_62K5HZ;
			else if (bw <= 125000) ifconf.bandwidth = BW_125KHZ;
			else if (bw <= 250000) ifconf.bandwidth = BW_250KHZ;
			else if (bw <= 500000) ifconf.bandwidth = BW_500KHZ;
			else ifconf.bandwidth = BW_UNDEFINED;
			ifconf.datarate = (uint32_t)json_object_dotget_number(conf, "chan_FSK.datarate");
			MSG("INFO: FSK channel enabled, radio %i selected, IF %i Hz, %u Hz bandwidth, %u bps datarate\n", ifconf.rf_chain, ifconf.freq_hz, bw, ifconf.datarate);
		}
		if (lgw_rxif_setconf(9, ifconf) != LGW_HAL_SUCCESS) {
			MSG("WARNING: invalid configuration for FSK channel\n");
		}
	}
	json_value_free(root_val);
	return 0;
}

int parse_gateway_configuration(const char * conf_file) {
	const char conf_obj[] = "gateway_conf";
	JSON_Value *root_val;
	JSON_Object *root = NULL;
	JSON_Object *conf = NULL;
	unsigned long long ull = 0;
	
	/* try to parse JSON */
	root_val = json_parse_file_with_comments(conf_file);
	root = json_value_get_object(root_val);
	if (root == NULL) {
		MSG("ERROR: %s id not a valid JSON file\n", conf_file);
		exit(EXIT_FAILURE);
	}
	conf = json_object_get_object(root, conf_obj);
	if (conf == NULL) {
		MSG("INFO: %s does not contain a JSON object named %s\n", conf_file, conf_obj);
		return -1;
	} else {
		MSG("INFO: %s does contain a JSON object named %s, parsing gateway parameters\n", conf_file, conf_obj);
	}
	
	/* getting network parameters (only those necessary for the packet logger) */
	sscanf(json_object_dotget_string(conf, "gateway_ID"), "%llx", &ull);
	lgwm = ull;
	MSG("INFO: gateway MAC address is configured to %016llX\n", ull);
	
	json_value_free(root_val);
	return 0;
}

static void sig_handler(int sigio) {
	if (sigio == SIGQUIT) {
		quit_sig = 1;;
	} else if ((sigio == SIGINT) || (sigio == SIGTERM)) {
		exit_sig = 1;
	}
}

/* describe command line options */
void usage(void) {
	printf("*** Library version information ***\n%s\n\n", lgw_version_info());
	printf( "Available options:\n");
	printf( " -h print this help\n");
	printf( " -r choose result file name\n");
}

/* compare router id and device id and returns received message type */
int compare_id(struct lgw_pkt_rx_s *p) {
	
	int j;
	char router_id[16 + 1];
	char device_id[16 + 1];

	if (p->status != STAT_CRC_OK)
		return INVALID_MSG;
	else {
		for (j = 1; j <= 8; ++j) {
			sprintf(router_id + 2*(j-1), "%02X", p->payload[j]);
		}		
		for (j = 9; j <= 16; ++j) {
			sprintf(device_id + 2*(j-9), "%02X", p->payload[j]);
		}
		
		if (strcmp(device_id, DEVICE_ID) || strcmp(router_id, ROUTER_ID))
			return INVALID_MSG;

		switch (p->payload[0]) {
			case 0:
				return JOIN_REQ_MSG;
			case 1:
				return TEST_MSG;
			case 2:
				return END_TEST_MSG;
			case 3:
				return ALL_TESTS_ENDED_MSG;
			default:
				return INVALID_MSG;
		}
	}
}

void write_results(int counter, struct lgw_pkt_rx_s* p) {
	
    float average_snr = 0;
    for (int i = 0; i < counter; i++)
        average_snr += snr[i];
    average_snr /= counter;  
	
    float variance_snr = 0;
    for (int i = 0; i < counter; i++)
        variance_snr += pow((snr[i] - average_snr), 2);
    variance_snr /= counter;

    float std_dev_snr = sqrt(variance_snr);

    if (result_file != NULL)
    {

		fprintf(result_file, "%+4.1f,", average_snr); // SNR
		fprintf(result_file, "%i,", counter); // Number of packets received

		// crc
		switch (p->payload[17]) {
			case 0:	fputs("4/5,", result_file); break;
			case 1:	fputs("2/3,", result_file); break;
			case 2:	fputs("4/7,", result_file); break;
			case 3:	fputs("1/2,", result_file); break;
			default: fputs("ERR,", result_file);			
		}
		
		// datarate
		switch (p->payload[18]) {
			case 5 :	fputs("SF7,", result_file); break;
			case 4 :	fputs("SF8,", result_file); break;
			case 3 :	fputs("SF9,", result_file); break;
			case 2 :	fputs("SF10,", result_file); break;
			case 1 :	fputs("SF11,", result_file); break;
			case 0 :	fputs("SF12,", result_file); break;
			case 6 :	fputs("undefined,", result_file); break;
			default: fputs("ERR,", result_file);
		}
		
		// bandwidth
		switch(p->payload[19]) {
			case 0 :	fputs("125,", result_file); break;
			case 1 :	fputs("250,", result_file); break;
			case 2:	    fputs("500,", result_file); break;
			case 3:     fputs("0,", result_file); break;
			default: fputs("-1,", result_file);
		}

		fprintf(result_file, "%i,", p->payload[20]); // power

		int average_time = p->payload[21] + (p->payload[22] <<8) + (p->payload[23] <<16) + (p->payload[24] <<24);
		fprintf(result_file, "%i,", average_time); // average tx time

		fprintf(result_file, "%i,", size); // packet size

		fprintf(result_file, "%i,", p->payload[25]); // messages per setting

		fprintf(result_file, "%i,", p->payload[26]); // test type

		int std_dev_time = p->payload[27] + (p->payload[28] <<8) + (p->payload[29] <<16) + (p->payload[30] <<24);
		fprintf(result_file, "%i,", std_dev_time); // standard deviation of tx time

		fprintf(result_file, "%+4.1f", std_dev_snr); // standard deviation of SNR

		fputs("\n", result_file);
    }
}

void send_join_response(struct lgw_pkt_rx_s* received) {
 
	struct lgw_pkt_tx_s join_response;
	
	join_response.freq_hz = JOIN_RESPONSE_FREQ;
	join_response.tx_mode = TIMESTAMPED;
	join_response.count_us = received->count_us + JOIN_RESPONSE_DELAY;
	join_response.rf_chain = JOIN_RF_CHAIN;
	join_response.rf_power = JOIN_RESPONSE_POWER;
	join_response.modulation = MOD_LORA;
	join_response.bandwidth = BW_125KHZ;
	join_response.datarate = DR_LORA_SF12;
	join_response.coderate = CR_LORA_4_5;
	join_response.invert_pol = true;
	// join_response.f_dev: only for FSK 
	join_response.preamble = 8; 
	join_response.no_crc = false;
	join_response.no_header = false;
	join_response.size = 3;
	join_response.payload[0]= 0;
	join_response.payload[1]= 1; 
	join_response.payload[2]= 2;
	lgw_send(join_response);
}

void openResultFile() {
    result_file = fopen(result_file_name, "w");
    if (result_file == NULL) {
    	MSG("ERROR: could not open result file.\n");
    	return;
    }
    fputs("snr,pkt_count,crc,dr,bw,pow,avg_time,size,msgs_per_setting,test_type,std_dev_time,std_dev_snr\n", result_file);
}

// MAIN FONCTION

int main(int argc, char **argv)
{
	int i; /* loop and temporary variables */
	struct timespec sleep_time = {0, 3000000}; /* 3 ms */
	
	int packet_counter = 0;
	
	/* allocate memory for packet fetching and processing */
	struct lgw_pkt_rx_s rxpkt[16]; /* array containing up to 16 inbound packets metadata */
	struct lgw_pkt_rx_s *p; /* pointer on a RX packet */
	int nb_pkt;

	configure_gateway();

	/* parse command line options */
	while ((i = getopt (argc, argv, "hr:")) != -1) {
		switch (i) {
			case 'h':
				usage();
				return EXIT_FAILURE;
				break;
			case 'r':
				result_file_name = optarg;
				break;
			
			default:
				MSG("ERROR: argument parsing use -h option for help\n");
				usage();
				return EXIT_FAILURE;
		}
	}

	/* configure signal handling */
	sigemptyset(&sigact.sa_mask);
	sigact.sa_flags = 0;
	sigact.sa_handler = sig_handler;
	sigaction(SIGQUIT, &sigact, NULL);
	sigaction(SIGINT, &sigact, NULL);
	sigaction(SIGTERM, &sigact, NULL);

	/* starting the concentrator */
	i = lgw_start();
	if (i == LGW_HAL_SUCCESS) {
		MSG("INFO: concentrator started, packet can now be received\n");
	} else {
		MSG("ERROR: failed to start the concentrator\n");
		return EXIT_FAILURE;
	}

	openResultFile();
	
	/* transform the MAC address into a string */
	sprintf(lgwm_str, "%08X%08X", (uint32_t)(lgwm >> 32), (uint32_t)(lgwm & 0xFFFFFFFF));

	/* main loop */
	while ((quit_sig != 1) && (exit_sig != 1)) {
		/* fetch packets */
		nb_pkt = lgw_receive(ARRAY_SIZE(rxpkt), rxpkt);
		if (nb_pkt == LGW_HAL_ERROR) {
			MSG("ERROR: failed packet fetch, exiting\n");
			return EXIT_FAILURE;
		} else if (nb_pkt == 0) {
			clock_nanosleep(CLOCK_MONOTONIC, 0, &sleep_time, NULL); /* wait a short time if no packets */
		} else {
			/* local timestamp generation until we get accurate GPS time */
		}
		
		/* log packets */
		for (i=0; i < nb_pkt; ++i) {
			p = &rxpkt[i];

			switch(compare_id(p)) {
				case JOIN_REQ_MSG:
					MSG("Sending join response.\n");
					send_join_response(p);
					packet_counter = 0;
					size = 0;
					break;
				case TEST_MSG:
					snr[packet_counter] = p->snr;
					packet_counter++;
					size = p->size;
					break;
				case END_TEST_MSG:
					if (packet_counter != 0) {				
						write_results(packet_counter, p);
						MSG("Ended series: %i packets received.\n", packet_counter);
					}
					packet_counter = 0;
					size = 0;
					break;
				case ALL_TESTS_ENDED_MSG:
					exit_sig = 1; // ending program
					MSG("All tests have been finished.\n");
					break;
				default:
					// message not recognized
					break;
			}
		}
	}
	
	if (exit_sig == 1) {
		/* clean up before leaving */
		i = lgw_stop();
		if (i == LGW_HAL_SUCCESS) {
			MSG("INFO: concentrator stopped successfully\n");
		} else {
			MSG("WARNING: failed to stop concentrator successfully\n");
		}
	}

	fclose(result_file);
	
	MSG("INFO: Exiting uplink concentrator program\n");
	return EXIT_SUCCESS;
}