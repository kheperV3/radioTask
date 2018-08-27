/*
moteur de la radio
tache de fond periodique (0.5 sec)
elle se synchronise avec la partie conversationnelle Snips
par 5 fichiers dans le repertoire /var/lib/snips/skills:
- link ==> lien vers la radio en cours
- volume ==> volume en cours
- live ==> etat de fonctionnement de la radio
		    = 0     arrêt
                    = 1     1ere station demandée
                    = 2     nouvelle station demandée
                    = 3     nouveau volume demandé
                    = 4     station en cours de diffusion
                    = 5     arrêt immédiat demandé (shutdown)
		    = 6     arrêt temporisé demandé (shutdown)
                    
- delay ==> delai en secondes avant l'arrêt
- session ==> nouvelle session demandée
		    = 0     pas de demande (bouton off)
		    = 1     demande faite (bouton on)
		    = 2     demande traitee
*/
#include <stdio.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdint.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <vlc/vlc.h>
#include <sys/socket.h>
#define TESTON  0
libvlc_instance_t* inst;
libvlc_media_player_t  *mp;
libvlc_media_t *m;



#define HIGH  1
#define LOW   0
#define LED  25
#define BUT  23
#define MSK23  0x00800000
#define MSK25  0x02000000

#define SEL 2         // registre no 2 mode pour les bits 20 à 29
#define SET 7         // registre no 7 mise à 1 pour les bits 0 à 31
#define CLR 10        // registre no 10 mise à 0 pour les bits 0 à 31
#define LEV 13        // registre no 13 niveau pour les bits 0 à 31
#define PUD 37        // registre no 37 nature du pullup/pulldown
#define PUDCLK  38    // registre no 38 bits en pullup/pulldown (0 à 31)


#define MSKSEL23  0xFFFFF1FF
#define MSKSEL25  0xFFFC7FFF
#define PULLUP 2
#define MSKOUT25  0x00008000



//definition des operations simples GPIO
#define ledOUT  *sel = ((*sel)& MSKSEL25) | MSKOUT25
#define butIN   *sel = (*sel) & MSKSEL23
#define butINPUP *sel = (*sel) & MSKSEL23;*pud = 2;*pudclk = MSK23
#define but_pullup *pud = 2; *pudclk = MSK23
#define ledON   *set = MSK25
#define ledOFF  *clr = MSK25
#define butVAL    (((*lev) & MSK23) ? HIGH : LOW)

volatile uint32_t *sel, *set, *clr, *lev, *pud, *pudclk,*gpio, *timer, *interrupt;
int hardwareSetup(void);
void printTest(void);
int live;
int hotword;
int volume;
int delay;
int session;




int main()
{
int p;
char b[80];
int n;
char link[120];

inst = libvlc_new(0,NULL);

hardwareSetup();

system ("touch /var/lib/snips/skills/live");
system ("touch /var/lib/snips/skills/link"); 
system ("touch /var/lib/snips/skills/volume");
system ("touch /var/lib/snips/skills/delay"); 
system ("touch /var/lib/snips/skills/session");
 
system("sudo chmod 666 /var/lib/snips/skills/live");
system("sudo chmod 666 /var/lib/snips/skills/link");
system("sudo chmod 666 /var/lib/snips/skills/volume");
system("sudo chmod 666 /var/lib/snips/skills/delay");
system("sudo chmod 666 /var/lib/snips/skills/session");
	
//intialisation des gpio 23 et 25
ledOUT;
butINPUP;

p = open("/var/lib/snips/skills/live",O_WRONLY);
n = write(p,"0000",5);
close(p);
p = open("/var/lib/snips/skills/volume",O_WRONLY);
n = write(p,"0005",5);
close(p);
p = open("/var/lib/snips/skills/link",O_WRONLY);
n = write(p,"dummy",6);
close(p);
p = open("/var/lib/snips/skills/delay",O_WRONLY);
n = write(p,"0000",5);
close(p);
p = open("/var/lib/snips/skills/session",O_WRONLY);
n = write(p,"0000",5);
close(p);
	
hotword = 1;
volume = 50;
delay = -1000;





for(;;)
{
usleep(500000);
p = open("/var/lib/snips/skills/session",O_RDONLY);
n = read(p,b,4);
close(p);
sscanf(b,"%d",&session);
	
if((butVAL == 0) && (session == 0))
   {
    p = open("/var/lib/snips/skills/session",O_WRONLY);
    n = write(p,"0001",5);
    close(p);   
   }
if((butVAL == 1) && (session == 2))
   {
    p = open("/var/lib/snips/skills/session",O_WRONLY);
    n = write(p,"0000",5);
    close(p);   
   }
	
	

p = open("/var/lib/snips/skills/live",O_RDONLY);
n = read(p,b,4);
close(p);
sscanf(b,"%d",&live);

printTest();
switch(live)
{


//##################
//#etat 0 arrêt
//##########################
case 0:
        ledOFF;
	break;

//###########
//#état 6 arrêt temporisé demandé
//#################
case 6:
//initialisation du delai par lecture du fichier delay
	p = open("/var/lib/snips/skills/delay",O_RDONLY);
	n = read(p,b,8);
	close(p);
	sscanf(b,"%d",&delay);
        delay = delay + delay;
//passage à l'état 4
	p = open("/var/lib/snips/skills/live",O_WRONLY);
        n = write(p,"0004",5);
        close(p);
	break;

//################
//# état 5 arrêt immédiat demandé
//##################
case 5:
        system("sudo shutdown now");
/*
//arrêt du player led éteint
	libvlc_media_player_stop(mp);
	ledOFF;
//remise en route  de la détection du hot word
	if(hotword==0)
		{
		hotword=1;
                system("sudo systemctl start snips-hotword");
		}
// état = 0
	p = open("/var/lib/snips/skills/live",O_WRONLY);
        n = write(p,"0000",5);
        close(p);
	break;
*/
//##################
//# état 4  en train de jouer
//####################
case 4:
	
//led allumée  et  détection hotword invalidée sauf si le bouton est poussé (+ volume à 0)
        ledON;
        if(butVAL == 0)
                {
                ledOFF;
                if(hotword==0)
                        {
                        hotword=1;
                        system("sudo systemctl start snips-hotword");
                        libvlc_audio_set_volume(mp,0);
                        }               
                }
        else
                {
                ledON;
                if(hotword==1)
                        {
                        hotword=0;
                        system("sudo systemctl stop snips-hotword");
                        libvlc_audio_set_volume(mp,volume);
                        }
                }
//si un arrêt temporisé en cours 
// delai decrémenté
//quand == 0 : état 5 arrêt immédiat
	if (delay > 0) delay--;
        if (delay == 0)
                {
                p = open("/var/lib/snips/skills/live",O_WRONLY);
                 n = write(p,"0005",5);
                close(p);
                delay = -1000;
                }

	break;

//#################
//#etat 2 nouvelle station
//#########################
case 2:
//# arrêt du player en cours
	libvlc_media_player_stop(mp);

//############################
//#état 1  1ere station
//###########################
case 1:
//demarrage d'un player avec le lien transmis dans link
	p = open("/var/lib/snips/skills/link",O_RDONLY);
	n = read(p,link,120);
	close(p);
	link[n] = 0;
	m = libvlc_media_new_location(inst,link);
	mp = libvlc_media_player_new_from_media(m);
	libvlc_media_release(m);
	libvlc_media_player_play(mp);

//#####################
//#état 3 changement de volume demandé
//######################
case 3:
//#récupération du volume dans le fichier d'interface
        p = open("/var/lib/snips/skills/volume",O_RDONLY);
        n = read(p,b,5);
        close(p);
        sscanf(b,"%d",&volume);
        volume = volume *10;
//modification du volume
	libvlc_audio_set_volume(mp,volume);
//passage à l'état 4
	p = open("/var/lib/snips/skills/live",O_WRONLY);
	n = write(p,"0004",5);
	close(p);

}

}
}

int hardwareSetup(void)
{
#define BCM2708_PERI_BASE       0x3F000000
#define GPIO_BASE               (BCM2708_PERI_BASE + 0x200000)
#define TIMER_BASE              0x20003000
#define INT_BASE                0x2000B000
      int memfd;
      unsigned int timend;
      void *gpio_map,*timer_map,*int_map;

      memfd = open("/dev/mem",O_RDWR|O_SYNC);
      if(memfd < 0)
        {
        printf("Mem open error\n");
        return(0);
        }

      gpio_map = mmap(NULL,4096,PROT_READ|PROT_WRITE,
                      MAP_SHARED,memfd,GPIO_BASE);

      timer_map = mmap(NULL,4096,PROT_READ|PROT_WRITE,
                      MAP_SHARED,memfd,TIMER_BASE);

      int_map = mmap(NULL,4096,PROT_READ|PROT_WRITE,
                      MAP_SHARED,memfd,INT_BASE);

      close(memfd);

      if(gpio_map == MAP_FAILED ||
         timer_map == MAP_FAILED ||
         int_map == MAP_FAILED)
        {
        printf("Map failed\n");
        return(0);
        }
                  // interrupt pointer
      interrupt = (volatile unsigned *)int_map;
                  // timer pointer
      timer = (volatile unsigned *)timer_map;
//      ++timer;    // timer lo 4 bytes
                  // timer hi 4 bytes available via *(timer+1)

                  // GPIO pointers
      gpio = (volatile unsigned *)gpio_map;

   sel = (uint32_t *) (gpio + SEL);
   set = (uint32_t *) (gpio + SET);
   clr = (uint32_t *) (gpio + CLR);
   lev = (uint32_t *) (gpio + LEV);
   pud = (uint32_t *) (gpio + PUD);
   pudclk = (uint32_t *) (gpio + PUDCLK);

   return 0;
}

void printTest()
{
if( TESTON == 1)
	{
	printf("\n %d  %d  %d  \n",live,hotword,delay); 
	}
}
  
