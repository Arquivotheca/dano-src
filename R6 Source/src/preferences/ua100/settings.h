#ifndef USETTINGS_H
#define USETTINGS_H

#define U_LISTEN 'ulis'
#define U_USE_MIC1 'umic'
#define U_USE_LINE 'ulin'
#define U_EFF_LOOP 'ueff'
#define U_KARAOKE 'ukar'

extern void listen(int fd);
extern void use_mic1(int fd);
extern void use_line(int fd);
extern void eff_loop(int fd);
extern void karaoke(int fd);

#endif
