#include <stdio.h>
#include <stdlib.h>

#include "rmail.h"

/* Kleines Testprogramm, welches Sprache, Subject und MsgID aller Mail
 * ausgibt, die im Index eines RMAIL Files gespeichert sind.
 */

void show(mail m) {
  printf("Lang: %c\n",m.header.language);
  printf("St. : %i\n",m.header.mailstart);
  printf("Leng: %i\n",m.header.maillen);
  printf("Subj: %s\n",m.subject);
  printf("MsID: %s\n",m.msgid);
  printf("\n");
}

void main(int argc, char **argv) {
  mailidx mi;
  mail m;
  int ret;

  if (argc != 2) {
    printf("Usage: %s <rmail-Filename>\n",argv[0]);
    exit(1);
  }

  ret = getmailfirst(argv[1],&mi,&m);

  if (ret) {
    printf("Index fÅr %s nicht gefunden!\n",argv[1]);
    exit(1);
  };

  do {
    show(m);
    ret = getmailnext(mi,&m);
  } while(!ret);

  getmailclose(mi);
}
