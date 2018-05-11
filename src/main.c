#define DEBUG_MSG 1

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <linux/input.h>
#include <sqlite3.h>

#include <dbgmsg.h>
#include <pthread.h>
#include <screen/tft.h>
#include <kbd/kbd.h>
#include <screen/draw.h>

#include "events.h"
#include "states.h"
#include "ev_sock.h"
#include "card.h"
#include "thread.h"
#include "thread_mifare.h"

#if defined(RUN_ON_PC)
#warning "This is a truncated assembly and some features of the program are disabled."
#endif // #if defined(RUN_ON_PC)

#if defined(RUN_ON_PC)
#define DEV_KBD_FB      "/dev/input/event1" // PC
#define DEV_MIFARE      "/dev/ttyACM0"
#else // #if defined(RUN_ON_PC)
#define DEV_KBD_FB      "/dev/input/event2"
#define DEV_MIFARE      "/dev/ttyO5"
#endif // #if defined(RUN_ON_PC)

#define CAN_PORT        "can0"
#define DEV_SCRN_FB     "/dev/fb0"          // TFT-дисплей
#define PATH2FONTS      "/mnt/data/fonts"   // шрифты

// for comunications
#define PORT_NO 3033

// for file transfer
// using:
//      echo -en "./new_usk04\0" | nc 192.168.3.179 3034
//      nc -v 192.168.3.179 3034 < ./usk04
#define PORT_NO_FT 3034

#define PATH_AF_MIFARE "/tmp/usk_04_af_sock"

static void initialize (app_data_t *app);
static void deinitialize (app_data_t *app);
static void setup_serial_port (int fd);

app_data_t *
get_app_data (ev_loop *loop) {
    app_data_t *app = ev_userdata (loop);
    assert (app);
    return app;
}


#include <prnt/prnt.h>
#include "iniio.h"

int main (void)
{
    printf("ini_load\n");
    ini_t* ini = ini_load ("test.ini");
    printf("ini_print\n");
    ini_print(ini);
    printf("ini_save\n");
    ini_save (ini, "test2.ini");
    printf("end\n");

    return 0;
    printf("\n\t ==================================\n");
    printf(  "\t|   USK-04 application ver. 0.0    |\n");
    printf(  "\t ==================================\n\n");

    app_data_t app = { 0 };
    initialize (&app);

#ifndef RUN_ON_PC
    // инициализация экрана
    app.screen = tft_open (DEV_SCRN_FB);
    if (!app.screen)
        return 1;//*/
    printf("tft opened...\n");
    // загрузка bitmap'ов в ОЗУ, инициализация таблицы bitmap'ов
    //ds_bm_init();
    // загрузка шрифтов (TODO; временно, перейти на векторные шрифты)
    fnt_t *fnt88 = fnt_open(PATH2FONTS"/8X8WIN1251.FNT", 8, 8);
    fnt_t *fnt816 = fnt_open(PATH2FONTS"/8X16WIN1251.FNT", 8, 16);//*/
    printf("fnt opened...\n");
    // TODO; временно
    tft_set_font (app.screen, fnt816);
    // клавиатура
    if (kbd_init (DEV_KBD_FB) != 0) {
        errmsg("kbd not initialized...\n");
    }
#endif
    // mifare reader
    app.mifare_fd = open (DEV_MIFARE, O_RDWR | O_NOCTTY /*| O_NONBLOCK*/);
    if (app.mifare_fd < 0) {
        perrmsg ("mifare open");
    } else
        setup_serial_port (app.mifare_fd);

    // сокет
    app.socket = sock_init (PORT_NO);
    if (app.socket > 0) {
        dbgmsg("Main socket started...      [%d]\n", app.socket);
    }
    // TODO: служебный сокет
    app.socket_srvc = sock_init (PORT_NO_FT);
    if (app.socket_srvc > 0) {
        dbgmsg("Service socket started...   [%d]\n", app.socket_srvc);
    }

    app.socket_can = sock_can_init (CAN_PORT);
    if (app.socket_can > 0) {
        printf("CAN started...              [%d]\n", app.socket_can);
    }

    /*app.socket_af = sock_af_init (PATH_AF);
    if (app.socket_af > 0) {
        dbgmsg("AF socket started...    [%d]\n", app.socket_af);
    }//*/

    // fsm
    app.fsm = setup_fsm (&app);

    if (!fsm_is_valid (app.fsm)) {
        printf ("fsm is invalid\n");
        goto fail; // return 1;
    }

    // инициализация libev
    app.loop = ev_default_loop (EVFLAG_AUTO);
    ev_set_userdata (app.loop, &app);
    setup_events (&app);

    // async TODO: не нужен
    /*if (app.mifare_fd > 0)
        async_init (&app.async, app.loop, app.mifare_fd);//*/

    // mifare thread
    pthread_mutex_t mifare_mutex = PTHREAD_MUTEX_INITIALIZER;
    app.mifare_mutex = &mifare_mutex;

    pthread_t thread_mifare = (pthread_t) NULL;
    if (pthread_create (&thread_mifare, NULL, process_mifare, &app) != 0) {
        dbgmsg ("'Mifare' thread not started...\n");
    }

    // тест принтера
    /*if (prnt_init() == ERR_PRNT_OK) {
        dbgmsg ("Printer OK\n");
        prnt_feed_paper(0, 1);
        prnt_str("ПРИВЕТ ПРИНТЕР!");
        prnt_feed_paper(0, 2);
        int error;
        dbgmsg ("Printer status:\n");
        prnt_status(&error);
        dbgmsg ("Printer done!\n");
    } else {
        dbgmsg ("Printer fail\n");
    }//*/

    //==========================================
    // run
    fsm_start (app.fsm);
    ev_run (app.loop, 0);
    //==========================================
fail:
    printf("deinitialize\n");

    //async_close (&app.async, NULL);

    /*if (app.socket_af > 0) {
        sock_close (app.socket_af);
        // See
        // https://stackoverflow.com/questions/15716302/so-reuseaddr-and-af-unix
        //
        // The path name, which must be specified in the sun.sun_path component,
        // is created as a file in the file system using bind(). The process that
        // calls bind() must therefore have write rights to the directory in which
        // the file is to be written. The system does not delete the file. It should
        // therefore be deleted by the process when it is no longer required.
        if (unlink (PATH_AF) != 0) {
            perrmsg ("Error unlinking the socket node");
        }
    }//*/

    // Always join or detach after canceling a thread
    if (pthread_cancel (thread_mifare) == 0) {
        dbgmsg ("joining thread...\n");
        pthread_join (thread_mifare, NULL);
    } else {
        perrmsg ("pthread_cancel(): faill\n");
    }

    if (app.socket_can > 0) {
        if (close (app.socket_can) < 0) {
            perrmsg ("can close() failed");
        }
    }

    sock_close (app.socket_srvc);
    sock_close (app.socket);

    if (app.mifare_fd > 0)
        close (app.mifare_fd);

#ifndef RUN_ON_PC
    kbd_close ();
    if (fnt88)
        fnt_close (fnt88);
    if (fnt816)
        fnt_close (fnt816);
    tft_close (app.screen);
#endif

    deinitialize (&app);

    return 0;
}

static void
initialize (app_data_t *app) {
    // read INI file

}

static void
deinitialize (app_data_t *app) {

}

static void
setup_serial_port (int fd) {
    struct termios ios;

    tcgetattr (fd, &ios);
    cfmakeraw (&ios);
    cfsetspeed (&ios, B115200);
    tcsetattr (fd, TCSANOW, &ios);
}

/*
#ifdef  __USE_POSIX
#warning POSIX compatible
#else
#warning no support POSIX
#endif

#if (!defined __STDC_VERSION__ || __STDC_VERSION__ < 201112L)
#warning std is not C11, its C99 or earler!
#endif

#if !defined(__STDC_ANALYZABLE__)
#warning __STDC_ANALYZABLE__: not support Analyzability (Annex L)
#endif
#if !defined(__STDC_IEC_559__)
#warning __STDC_IEC_559__: not support IEC 60559 floating-point arithmetic (Annex F)
#endif
#if !defined(__STDC_IEC_559_COMPLEX__)
#warning __STDC_IEC_559_COMPLEX__: not support IEC 60559 compatible complex arithmetic (Annex G)
#endif
#if !defined(__STDC_LIB_EXT1__)
#warning __STDC_LIB_EXT1__: not support Bounds-checking interfaces (Annex K)
#endif
#if !defined(__STDC_NO_COMPLEX__)
#warning __STDC_NO_COMPLEX__: not support Complex types (<complex.h>)
#endif
#if !defined(__STDC_NO_THREADS__)
#warning __STDC_NO_THREADS__: not support Multithreading (<threads.h>)
#endif
#if !defined(__STDC_NO_ATOMICS__)
#warning __STDC_NO_ATOMICS__: not support Atomic primitives and types (<stdatomic.h> and the _Atomic type qualifier)
#endif
#if !defined(__STDC_NO_VLA__)
#warning __STDC_NO_VLA__: not support Variable length arrays
#endif
//*/