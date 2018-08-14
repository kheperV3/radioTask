# radioTask

On trouve ici le source (C) de la tache cyclique qui est un des composants de la radio à commandes vocales Ma Radio
composée :
- l'application Snips Radio draft ( 4 intents : selectStation, changeVolume, stopRadio et time)
- le programme Python réalisant les actions correspondantes : action-louisros.radio.py (=> https://github.com/kheperV3/radio)
- le programme de la tache qui interprete les actions précédentes : radio.c (=> https://github.com/kheperV3/radioTask)

Pour compiler radio.c il faut :
- disposer de la bibliotheque vlc : libvlc-dev
- executer la commande : cc -o radio -l vlc radio.c
- placer l'executable radio dans le repertoire /home/pi

Pour assurer le lancement automatique au boot de la tache radio il faut :
- ajouter la ligne suivante dans le fichier /etc/rc.local (sudo nano /etc/rc.local)
      /home/pi/radio&
- rebooter
