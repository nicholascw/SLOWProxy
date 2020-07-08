#ifndef SLOWPROXY_LOG_H
#define SLOWPROXY_LOG_H
#define __FNAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#define L_DEBUG(STR) (slow_print_log(DBG, __FUNCTION__, STR, __FNAME__, __LINE__))
#define L_INFO(STR) (slow_print_log(INFO, __FUNCTION__, STR, __FNAME__, __LINE__))
#define L_WARN(STR) (slow_print_log(WARN, __FUNCTION__, STR, __FNAME__, __LINE__))
#define L_ERR(STR) (slow_print_log(ERR, __FUNCTION__, STR, __FNAME__, __LINE__))
#define L_PERROR() (slow_perror_log(__FUNCTION__, __FNAME__, __LINE__))

typedef enum {
    DBG = 1, INFO = 2, WARN = 3, ERR = 4, SHUT = 5
} loglevel_t;

void slow_set_loglevel(loglevel_t level);

void slow_perror_log(const char *func_name, const char *src_name, unsigned int lineno);

void slow_print_log(loglevel_t lvl, const char *func_name, const char *err_string,
                    const char *src_name, unsigned int lineno);

#endif //SLOWPROXY_LOG_H
