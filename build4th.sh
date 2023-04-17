#!/bin/bash
 
CC="gcc -m32"

# CFLAGS="-g -O0 -DPF_SUPPORT_FP -DPF_STATIC_DIC -D__CMS__"
  CFLAGS="-g -O0 -DPF_SUPPORT_FP -DPF_STATIC_DIC"

${CC} -c ${CFLAGS} pf_cglue.c
${CC} -c ${CFLAGS} pf_clib.c
${CC} -c ${CFLAGS} pf_core.c
${CC} -c ${CFLAGS} pf_inner.c
${CC} -c ${CFLAGS} pf_io.c
${CC} -c ${CFLAGS} pf_main.c
${CC} -c ${CFLAGS} pf_mem.c
${CC} -c ${CFLAGS} pf_save.c
${CC} -c ${CFLAGS} pf_text.c
${CC} -c ${CFLAGS} pf_words.c
${CC} -c ${CFLAGS} pfcompil.c
${CC} -c ${CFLAGS} pfcustom.c
 
${CC} -o pforth *.o -lm
