#ifndef __SIM5320_H__
#define __SIM5320_H__
enum https_state_enum {
	HTTPS_STATE_NONE,
	HTTPS_STATE_ACCQUIRED_HTTPS,
	HTTPS_STATE_OPENING_NETWORK,
	HTTPS_STATE_CLOSING_NETWORK,
	HTTPS_STATE_OPENED_NETWORK,
	HTTPS_STATE_CLOSING_SESSION,
	HTTPS_STATE_OPENING_SESSION,
	HTTPS_STATE_OPENED_SESSION
};

extern const char at_cmd_ate_off[];
extern const char at_cmd_ate_on[];

extern const char at_cmd_https_state[];

extern const char at_cmd_https_start[];
extern const char at_cmd_https_stop[];

extern const char at_cmd_https_open[];
extern const char at_cmd_https_close[];

extern const char at_cmd_https_send[];
extern const char at_cmd_https_recv[];

extern const char answ_ok[];
extern const char answ_error[];

extern char answer[256];


bool at_cmd(void (*callback)(void *), void *param, const char *format, ...);
bool at_wait(const char *str, const char *str2);
void send_string(void *hdr);
void recv_https(void *len);
FILE *gsm_init(void);
void https_state_callback(void *state);

#endif
