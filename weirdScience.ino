#include <FatReader.h>
#include <SdReader.h>
#include <avr/pgmspace.h>
#include "WaveUtil.h"
#include "WaveHC.h"
#include "ServoTimer2.h"
#define degreesToUS( _degrees) (_degrees * 6 + 900) // macro to convert degrees to microseconds

SdReader card;    // This object holds the information for the card
FatVolume vol;    // This holds the information for the partition on the card
FatReader root;   // This holds the information for the filesystem on the card

FatReader f;      // This holds the information for the file we're play
ServoTimer2 eyesLR;  // servo for eyes left right
ServoTimer2 eyesUD;  // servo for eyes up down
ServoTimer2 mouth;
int nowPlaying = 0;

uint8_t dirLevel; // indent level for file/dir names    (for prettyprinting)
dir_t dirBuf;     // buffer for directory reads

WaveHC wave;      // This is the only wave (audio) object, since we will only play one at a time

// Function definitions (we define them here, but the code is below)
void lsR(FatReader &d);
void play(FatReader &dir);

void setup() {
  Serial.begin(9600);           // set up Serial library at 9600 bps for debugging
  
  //putstring_nl("\nWave test!");  // say we woke up!
  
  pinMode(14,OUTPUT);
  pinMode(15,OUTPUT);
  pinMode(16,OUTPUT);
  pinMode(19,INPUT_PULLUP);
  eyesLR.attach(14);  // attaches the servo on pin 9 to the servo object 
  eyesUD.attach(15);  // attaches the servo on pin 9 to the servo object 
  mouth.attach(16);  // attfches the servo on pin 9 to the servo object (mouth closed = 90 degrees, open = 60)
  
  putstring("Free RAM: ");       // This can help with debugging, running out of RAM is bad
//  Serial.println(freeRam());  
 
  // Set the output pins for the DAC control. This pins are defined in the library
  pinMode(2, OUTPUT);
  pinMode(3, OUTPUT);
  pinMode(4, OUTPUT);
  pinMode(5, OUTPUT);
  
  //  if (!card.init(true)) { //play with 4 MHz spi if 8MHz isn't working for you
  if (!card.init()) {         //play with 8 MHz spi (default faster!)  
    //putstring_nl("Card init. failed!");  // Something went wrong, lets print out why
    //sdErrorCheck();
    while(1);                            // then 'halt' - do nothing!
  }
  
  // enable optimize read - some cards may timeout. Disable if you're having problems
  card.partialBlockRead(true);
  
  // Now we will look for a FAT partition!
  uint8_t part;
  for (part = 0; part < 5; part++) {     // we have up to 5 slots to look in
    if (vol.init(card, part)) 
      break;                             // we found one, lets bail
  }
  if (part == 5) {                       // if we ended up not finding one  :(
    //putstring_nl("No valid FAT partition!");
    //sdErrorCheck();      // Something went wrong, lets print out why
    while(1);                            // then 'halt' - do nothing!
  }
  

  // Try to open the root directory
  if (!root.openRoot(vol)) {
//    putstring_nl("Can't open root dir!"); // Something went wrong,
    while(1);                             // then 'halt' - do nothing!
  }
  
  // Whew! We got past the tough parts.
  //putstring_nl("Files found:");
  dirLevel = 0;
  // Print out all of the files in all the directories.
  lsR(root);
}

//////////////////////////////////// LOOP
void loop() { 
  
  if(digitalRead(19)==LOW){ 
     if(nowPlaying==0){
       root.rewind();                
       play(root);
       nowPlaying=1;
     }
     else {
        Serial.print("push but already playing\n");
     }    
  }
  else {
   //      Serial.print("push\n");
  }
  //Serial.print(nowPlaying);
  //Serial.print("\n");
  delay(10);
    //  root.rewind();                
//  play(root);

}

/////////////////////////////////// HELPERS


/*
 * list recursively - possible stack overflow if subdirectories too nested
 */
void lsR(FatReader &d)
{
  int8_t r;                     // indicates the level of recursion
  
  while ((r = d.readDir(dirBuf)) > 0) {     // read the next file in the directory 
    // skip subdirs . and ..
    if (dirBuf.name[0] == '.') 
      continue;
    
    for (uint8_t i = 0; i < dirLevel; i++) 
      Serial.print(' ');        // this is for prettyprinting, put spaces in front
   // printName(dirBuf);          // print the name of the file we just found
    Serial.println();           // and a new line
    
    if (DIR_IS_SUBDIR(dirBuf)) {   // we will recurse on any direcory
      FatReader s;                 // make a new directory object to hold information
      dirLevel += 2;               // indent 2 spaces for future prints
      if (s.open(vol, dirBuf)) 
        lsR(s);                    // list all the files in this directory now!
      dirLevel -=2;                // remove the extra indentation
    }
  }
  //sdErrorCheck();                  // are we doign OK?
}
/*
 * play recursively - possible stack overflow if subdirectories too nested
 */
void play(FatReader &dir)
{
  FatReader file;
  while (dir.readDir(dirBuf) > 0) {    // Read every file in the directory one at a time
    // skip . and .. directories
    if (dirBuf.name[0] == '.') 
      continue;
    
    Serial.println();            // clear out a new line
    
    for (uint8_t i = 0; i < dirLevel; i++) 
       Serial.print(' ');       // this is for prettyprinting, put spaces in front

    if (!file.open(vol, dirBuf)) {       // open the file in the directory
      Serial.println("file.open failed");  // something went wrong :(
      while(1);                            // halt
    }
    
    if (file.isDir()) {                    // check if we opened a new directory
      //putstring("Subdir: ");
      //printName(dirBuf);
      dirLevel += 2;                       // add more spaces
      // play files in subdirectory
      play(file);                         // recursive!
      dirLevel -= 2;    
    }
    else {
      // Aha! we found a file that isnt a directory
      //putstring("Playing "); printName(dirBuf);       // print it out
      if (!wave.create(file)) {            // Figure out, is it a WAV proper?
       // putstring(" Not a valid WAV");     // ok skip it
      } else {
        Serial.println();                  // Hooray it IS a WAV proper!
        wave.play();                       // make some noise!
        headMovement("weirdScience");       
        while (wave.isplaying) {           // playing occurs in interrupts, so we print dots in realtime          
        }
        Serial.print("just finished playing weird science\n");
     //   sdErrorCheck();                    // everything OK?
//        if (wave.errors)Serial.println(wave.errors);     // wave decoding errors
      }      
    }
  }
}

void playfile(char *name) {
  // see if the wave object is currently doing something
  if (wave.isplaying) {// already playing something, so stop it!
    wave.stop(); // stop it
  }
  // look in the root directory and open the file
  if (!f.open(root, name)) {
   // putstring("Couldn't open file "); Serial.print(name); return;
  }
  // OK read the file and turn it into a wave object
  if (!wave.create(f)) {
    //putstring_nl("Not a valid WAV"); return;
  }
  
  // ok time to play! start playback
  wave.play();
}

void headMovement(String songName) {
 
  if(songName == "weirdScience") {
    // Here's the sequence of eye and mouth movements for the song weird science  
    delay(790);
    eyesLR.write(degreesToUS(160));delay(316);eyesLR.write(degreesToUS(90));
    delay(21);
    eyesLR.write(degreesToUS(45));delay(194);eyesLR.write(degreesToUS(90));
    delay(31);
    eyesLR.write(degreesToUS(160));delay(194);eyesLR.write(degreesToUS(90));
    delay(51);
    eyesLR.write(degreesToUS(45));delay(346);eyesLR.write(degreesToUS(90));
    delay(83);
    eyesUD.write(degreesToUS(160));delay(418);eyesUD.write(degreesToUS(90));
    delay(41);
    eyesUD.write(degreesToUS(45));delay(397);eyesUD.write(degreesToUS(90));
    delay(53);
    eyesUD.write(degreesToUS(160));delay(162);eyesUD.write(degreesToUS(90));
    delay(53);
    eyesUD.write(degreesToUS(45));delay(264);eyesUD.write(degreesToUS(90));
    delay(62);
    eyesUD.write(degreesToUS(160));delay(113);eyesUD.write(degreesToUS(90));
    delay(0);
    eyesLR.write(degreesToUS(160));delay(346);eyesLR.write(degreesToUS(90));
    delay(42);
    eyesLR.write(degreesToUS(45));delay(408);eyesLR.write(degreesToUS(90));
    delay(10);
    eyesUD.write(degreesToUS(160));delay(397);eyesUD.write(degreesToUS(90));
    delay(32);
    eyesUD.write(degreesToUS(45));delay(214);eyesUD.write(degreesToUS(90));
    delay(51);
    eyesUD.write(degreesToUS(160));delay(245);eyesUD.write(degreesToUS(90));
    delay(11);
    eyesLR.write(degreesToUS(160));delay(437);eyesLR.write(degreesToUS(90));
    delay(22);
    eyesLR.write(degreesToUS(45));delay(438);eyesLR.write(degreesToUS(90));
    delay(21);
    eyesLR.write(degreesToUS(160));delay(417);eyesLR.write(degreesToUS(90));
    delay(32);
    eyesLR.write(degreesToUS(45));delay(224);eyesLR.write(degreesToUS(90));
    delay(21);
    eyesLR.write(degreesToUS(160));delay(163);eyesLR.write(degreesToUS(90));
    delay(21);
    eyesLR.write(degreesToUS(45));delay(235);eyesLR.write(degreesToUS(90));
    delay(215);
    mouth.write(degreesToUS(50));delay(303);mouth.write(degreesToUS(80));
    delay(215);
    mouth.write(degreesToUS(50));delay(112);mouth.write(degreesToUS(80));
    delay(102);
    mouth.write(degreesToUS(50));delay(384);mouth.write(degreesToUS(80));
    delay(328);
    eyesLR.write(degreesToUS(160));delay(438);eyesLR.write(degreesToUS(90));
    delay(61);
    eyesLR.write(degreesToUS(45));delay(367);eyesLR.write(degreesToUS(90));
    delay(93);
    eyesUD.write(degreesToUS(160));delay(397);eyesUD.write(degreesToUS(90));
    delay(62);
    eyesUD.write(degreesToUS(45));delay(469);eyesUD.write(degreesToUS(90));
    delay(10);
    eyesLR.write(degreesToUS(160));delay(438);eyesLR.write(degreesToUS(90));
    delay(32);
    eyesLR.write(degreesToUS(45));delay(478);eyesLR.write(degreesToUS(90));
    delay(12);
    eyesUD.write(degreesToUS(160));delay(387);eyesUD.write(degreesToUS(90));
    delay(10);
    eyesLR.write(degreesToUS(45));delay(20);eyesLR.write(degreesToUS(90));
    delay(12);
    eyesUD.write(degreesToUS(45));delay(438);eyesUD.write(degreesToUS(90));
    delay(10);
    eyesLR.write(degreesToUS(160));delay(449);eyesLR.write(degreesToUS(90));
    delay(30);
    eyesLR.write(degreesToUS(45));delay(377);eyesLR.write(degreesToUS(90));
    delay(544);
    eyesLR.write(degreesToUS(160));delay(428);eyesLR.write(degreesToUS(90));
    delay(532);
    mouth.write(degreesToUS(50));delay(263);mouth.write(degreesToUS(80));
    delay(1137);
    eyesLR.write(degreesToUS(160));delay(398);eyesLR.write(degreesToUS(90));
    delay(92);
    mouth.write(degreesToUS(50));delay(413);mouth.write(degreesToUS(80));
    delay(401);
    eyesLR.write(degreesToUS(45));delay(478);eyesLR.write(degreesToUS(90));
    delay(11);
    eyesUD.write(degreesToUS(160));delay(499);eyesUD.write(degreesToUS(90));
    delay(61);
    eyesUD.write(degreesToUS(45));delay(368);eyesUD.write(degreesToUS(90));
    delay(10);
    eyesLR.write(degreesToUS(160));delay(499);eyesLR.write(degreesToUS(90));
    delay(32);
    eyesLR.write(degreesToUS(45));delay(397);eyesLR.write(degreesToUS(90));
    delay(10);
    eyesUD.write(degreesToUS(160));delay(439);eyesUD.write(degreesToUS(90));
    delay(11);
    eyesLR.write(degreesToUS(45));delay(20);eyesLR.write(degreesToUS(90));
    delay(11);
    eyesUD.write(degreesToUS(45));delay(509);eyesUD.write(degreesToUS(90));
    delay(11);
    eyesLR.write(degreesToUS(160));delay(498);eyesLR.write(degreesToUS(90));
    delay(32);
    eyesLR.write(degreesToUS(45));delay(387);eyesLR.write(degreesToUS(90));
    delay(21);
    eyesLR.write(degreesToUS(160));delay(174);eyesLR.write(degreesToUS(90));
    delay(338);
    mouth.write(degreesToUS(50));delay(222);mouth.write(degreesToUS(80));
    delay(216);
    mouth.write(degreesToUS(50));delay(131);mouth.write(degreesToUS(80));
    delay(123);
    mouth.write(degreesToUS(50));delay(474);mouth.write(degreesToUS(80));
    delay(1167);
    mouth.write(degreesToUS(50));delay(122);mouth.write(degreesToUS(80));
    delay(103);
    mouth.write(degreesToUS(50));delay(132);mouth.write(degreesToUS(80));
    delay(164);
    mouth.write(degreesToUS(50));delay(111);mouth.write(degreesToUS(80));
    delay(113);
    mouth.write(degreesToUS(50));delay(91);mouth.write(degreesToUS(80));
    delay(144);
    mouth.write(degreesToUS(50));delay(90);mouth.write(degreesToUS(80));
    delay(114);
    mouth.write(degreesToUS(50));delay(121);mouth.write(degreesToUS(80));
    delay(124);
    mouth.write(degreesToUS(50));delay(222);mouth.write(degreesToUS(80));
    delay(533);
    mouth.write(degreesToUS(50));delay(111);mouth.write(degreesToUS(80));
    delay(62);
    mouth.write(degreesToUS(50));delay(91);mouth.write(degreesToUS(80));
    delay(113);
    mouth.write(degreesToUS(50));delay(132);mouth.write(degreesToUS(80));
    delay(153);
    mouth.write(degreesToUS(50));delay(172);mouth.write(degreesToUS(80));
    delay(287);
    mouth.write(degreesToUS(50));delay(101);mouth.write(degreesToUS(80));
    delay(114);
    mouth.write(degreesToUS(50));delay(121);mouth.write(degreesToUS(80));
    delay(144);
    mouth.write(degreesToUS(50));delay(100);mouth.write(degreesToUS(80));
    delay(114);
    mouth.write(degreesToUS(50));delay(111);mouth.write(degreesToUS(80));
    delay(144);
    mouth.write(degreesToUS(50));delay(121);mouth.write(degreesToUS(80));
    delay(359);
    mouth.write(degreesToUS(50));delay(121);mouth.write(degreesToUS(80));
    delay(93);
    mouth.write(degreesToUS(50));delay(111);mouth.write(degreesToUS(80));
    delay(124);
    mouth.write(degreesToUS(50));delay(221);mouth.write(degreesToUS(80));
    delay(237);
    mouth.write(degreesToUS(50));delay(120);mouth.write(degreesToUS(80));
    delay(114);
    mouth.write(degreesToUS(50));delay(373);mouth.write(degreesToUS(80));
    delay(759);
    mouth.write(degreesToUS(50));delay(453);mouth.write(degreesToUS(80));
    delay(482);
    mouth.write(degreesToUS(50));delay(111);mouth.write(degreesToUS(80));
    delay(82);
    mouth.write(degreesToUS(50));delay(172);mouth.write(degreesToUS(80));
    delay(154);
    mouth.write(degreesToUS(50));delay(111);mouth.write(degreesToUS(80));
    delay(103);
    mouth.write(degreesToUS(50));delay(122);mouth.write(degreesToUS(80));
    delay(143);
    mouth.write(degreesToUS(50));delay(202);mouth.write(degreesToUS(80));
    delay(277);
    mouth.write(degreesToUS(50));delay(111);mouth.write(degreesToUS(80));
    delay(83);
    mouth.write(degreesToUS(50));delay(121);mouth.write(degreesToUS(80));
    delay(134);
    mouth.write(degreesToUS(50));delay(121);mouth.write(degreesToUS(80));
    delay(93);
    mouth.write(degreesToUS(50));delay(121);mouth.write(degreesToUS(80));
    delay(123);
    mouth.write(degreesToUS(50));delay(252);mouth.write(degreesToUS(80));
    delay(236);
    mouth.write(degreesToUS(50));delay(122);mouth.write(degreesToUS(80));
    delay(82);
    mouth.write(degreesToUS(50));delay(121);mouth.write(degreesToUS(80));
    delay(134);
    mouth.write(degreesToUS(50));delay(131);mouth.write(degreesToUS(80));
    delay(134);
    mouth.write(degreesToUS(50));delay(110);mouth.write(degreesToUS(80));
    delay(135);
    mouth.write(degreesToUS(50));delay(101);mouth.write(degreesToUS(80));
    delay(113);
    mouth.write(degreesToUS(50));delay(101);mouth.write(degreesToUS(80));
    delay(133);
    mouth.write(degreesToUS(50));delay(112);mouth.write(degreesToUS(80));
    delay(92);
    mouth.write(degreesToUS(50));delay(122);mouth.write(degreesToUS(80));
    delay(143);
    mouth.write(degreesToUS(50));delay(202);mouth.write(degreesToUS(80));
    delay(247);
    mouth.write(degreesToUS(50));delay(120);mouth.write(degreesToUS(80));
    delay(114);
    mouth.write(degreesToUS(50));delay(283);mouth.write(degreesToUS(80));
    delay(1331);
    mouth.write(degreesToUS(50));delay(151);mouth.write(degreesToUS(80));
    delay(114);
    mouth.write(degreesToUS(50));delay(131);mouth.write(degreesToUS(80));
    delay(154);
    mouth.write(degreesToUS(50));delay(81);mouth.write(degreesToUS(80));
    delay(103);
    mouth.write(degreesToUS(50));delay(121);mouth.write(degreesToUS(80));
    delay(93);
    mouth.write(degreesToUS(50));delay(121);mouth.write(degreesToUS(80));
    delay(103);
    mouth.write(degreesToUS(50));delay(142);mouth.write(degreesToUS(80));
    delay(112);
    mouth.write(degreesToUS(50));delay(223);mouth.write(degreesToUS(80));
    delay(296);
    mouth.write(degreesToUS(50));delay(133);mouth.write(degreesToUS(80));
    delay(61);
    mouth.write(degreesToUS(50));delay(122);mouth.write(degreesToUS(80));
    delay(123);
    mouth.write(degreesToUS(50));delay(121);mouth.write(degreesToUS(80));
    delay(93);
    mouth.write(degreesToUS(50));delay(101);mouth.write(degreesToUS(80));
    delay(113);
    mouth.write(degreesToUS(50));delay(212);mouth.write(degreesToUS(80));
    delay(257);
    mouth.write(degreesToUS(50));delay(110);mouth.write(degreesToUS(80));
    delay(94);
    mouth.write(degreesToUS(50));delay(120);mouth.write(degreesToUS(80));
    delay(155);
    mouth.write(degreesToUS(50));delay(111);mouth.write(degreesToUS(80));
    delay(103);
    mouth.write(degreesToUS(50));delay(111);mouth.write(degreesToUS(80));
    delay(154);
    mouth.write(degreesToUS(50));delay(162);mouth.write(degreesToUS(80));
    delay(297);
    mouth.write(degreesToUS(50));delay(191);mouth.write(degreesToUS(80));
    delay(288);
    mouth.write(degreesToUS(50));delay(252);mouth.write(degreesToUS(80));
    delay(236);
    mouth.write(degreesToUS(50));delay(142);mouth.write(degreesToUS(80));
    delay(102);
    mouth.write(degreesToUS(50));delay(394);mouth.write(degreesToUS(80));
    delay(717);
    mouth.write(degreesToUS(50));delay(423);mouth.write(degreesToUS(80));
    delay(72);
    mouth.write(degreesToUS(50));delay(132);mouth.write(degreesToUS(80));
    delay(103);
    mouth.write(degreesToUS(50));delay(131);mouth.write(degreesToUS(80));
    delay(134);
    mouth.write(degreesToUS(50));delay(110);mouth.write(degreesToUS(80));
    delay(114);
    mouth.write(degreesToUS(50));delay(121);mouth.write(degreesToUS(80));
    delay(113);
    mouth.write(degreesToUS(50));delay(111);mouth.write(degreesToUS(80));
    delay(104);
    mouth.write(degreesToUS(50));delay(120);mouth.write(degreesToUS(80));
    delay(124);
    mouth.write(degreesToUS(50));delay(252);mouth.write(degreesToUS(80));
    delay(247);
    mouth.write(degreesToUS(50));delay(111);mouth.write(degreesToUS(80));
    delay(82);
    mouth.write(degreesToUS(50));delay(121);mouth.write(degreesToUS(80));
    delay(134);
    mouth.write(degreesToUS(50));delay(91);mouth.write(degreesToUS(80));
    delay(123);
    mouth.write(degreesToUS(50));delay(121);mouth.write(degreesToUS(80));
    delay(134);
    mouth.write(degreesToUS(50));delay(171);mouth.write(degreesToUS(80));
    delay(278);
    mouth.write(degreesToUS(50));delay(131);mouth.write(degreesToUS(80));
    delay(114);
    mouth.write(degreesToUS(50));delay(100);mouth.write(degreesToUS(80));
    delay(124);
    mouth.write(degreesToUS(50));delay(121);mouth.write(degreesToUS(80));
    delay(103);
    mouth.write(degreesToUS(50));delay(121);mouth.write(degreesToUS(80));
    delay(144);
    mouth.write(degreesToUS(50));delay(263);mouth.write(degreesToUS(80));
    delay(81);
    mouth.write(degreesToUS(50));delay(52);mouth.write(degreesToUS(80));
    delay(481);
    mouth.write(degreesToUS(50));delay(172);mouth.write(degreesToUS(80));
    delay(113);
    mouth.write(degreesToUS(50));delay(111);mouth.write(degreesToUS(80));
    delay(63);
    mouth.write(degreesToUS(50));delay(131);mouth.write(degreesToUS(80));
    delay(112);
    mouth.write(degreesToUS(50));delay(364);mouth.write(degreesToUS(80));
    delay(829);
    mouth.write(degreesToUS(50));delay(112);mouth.write(degreesToUS(80));
    delay(102);
    mouth.write(degreesToUS(50));delay(112);mouth.write(degreesToUS(80));
    delay(143);
    mouth.write(degreesToUS(50));delay(696);mouth.write(degreesToUS(80));
    delay(472);
    mouth.write(degreesToUS(50));delay(110);mouth.write(degreesToUS(80));
    delay(155);
    mouth.write(degreesToUS(50));delay(101);mouth.write(degreesToUS(80));
    delay(113);
    mouth.write(degreesToUS(50));delay(112);mouth.write(degreesToUS(80));
    delay(133);
    mouth.write(degreesToUS(50));delay(122);mouth.write(degreesToUS(80));
    delay(102);
    mouth.write(degreesToUS(50));delay(403);mouth.write(degreesToUS(80));
    delay(267);
    mouth.write(degreesToUS(50));delay(242);mouth.write(degreesToUS(80));
    delay(113);
    mouth.write(degreesToUS(50));delay(112);mouth.write(degreesToUS(80));
    delay(102);
    mouth.write(degreesToUS(50));delay(112);mouth.write(degreesToUS(80));
    delay(102);
    mouth.write(degreesToUS(50));delay(91);mouth.write(degreesToUS(80));
    delay(93);
    mouth.write(degreesToUS(50));delay(121);mouth.write(degreesToUS(80));
    delay(104);
    mouth.write(degreesToUS(50));delay(342);mouth.write(degreesToUS(80));
    delay(144);
    mouth.write(degreesToUS(50));delay(101);mouth.write(degreesToUS(80));
    delay(123);
    mouth.write(degreesToUS(50));delay(102);mouth.write(degreesToUS(80));
    delay(133);
    mouth.write(degreesToUS(50));delay(111);mouth.write(degreesToUS(80));
    delay(103);
    mouth.write(degreesToUS(50));delay(122);mouth.write(degreesToUS(80));
    delay(122);
    mouth.write(degreesToUS(50));delay(112);mouth.write(degreesToUS(80));
    delay(113);
    eyesLR.write(degreesToUS(45));delay(448);eyesLR.write(degreesToUS(90));
    delay(52);
    eyesLR.write(degreesToUS(160));delay(184);eyesLR.write(degreesToUS(90));
    delay(31);
    eyesLR.write(degreesToUS(45));delay(662);eyesLR.write(degreesToUS(90));
    delay(10);
    eyesUD.write(degreesToUS(160));delay(276);eyesUD.write(degreesToUS(90));
    delay(11);
    eyesLR.write(degreesToUS(160));delay(61);eyesLR.write(degreesToUS(90));
    delay(11);
    eyesUD.write(degreesToUS(45));delay(92);eyesUD.write(degreesToUS(90));
    delay(41);
    eyesUD.write(degreesToUS(160));delay(133);eyesUD.write(degreesToUS(90));
    delay(10);
    eyesLR.write(degreesToUS(160));delay(387);eyesLR.write(degreesToUS(90));
    delay(22);
    eyesLR.write(degreesToUS(45));delay(173);eyesLR.write(degreesToUS(90));
    delay(32);
    eyesLR.write(degreesToUS(160));delay(438);eyesLR.write(degreesToUS(90));
    delay(51);
    eyesLR.write(degreesToUS(45));delay(133);eyesLR.write(degreesToUS(90));
    delay(10);
    eyesUD.write(degreesToUS(160));delay(245);eyesUD.write(degreesToUS(90));
    delay(51);
    eyesUD.write(degreesToUS(45));delay(215);eyesUD.write(degreesToUS(90));
    delay(52);
    eyesUD.write(degreesToUS(160));delay(133);eyesUD.write(degreesToUS(90));
    delay(10);
    eyesLR.write(degreesToUS(160));delay(326);eyesLR.write(degreesToUS(90));
    delay(42);
    eyesLR.write(degreesToUS(45));delay(162);eyesLR.write(degreesToUS(90));
    delay(104);
    eyesLR.write(degreesToUS(160));delay(346);eyesLR.write(degreesToUS(90));
    delay(52);
    eyesUD.write(degreesToUS(160));delay(194);eyesUD.write(degreesToUS(90));
    delay(30);
    eyesUD.write(degreesToUS(45));delay(266);eyesUD.write(degreesToUS(90));
    delay(11);
    eyesLR.write(degreesToUS(160));delay(204);eyesLR.write(degreesToUS(90));
    delay(41);
    eyesLR.write(degreesToUS(45));delay(357);eyesLR.write(degreesToUS(90));
    delay(41);
    eyesLR.write(degreesToUS(160));delay(204);eyesLR.write(degreesToUS(90));
    delay(52);
    eyesLR.write(degreesToUS(45));delay(285);eyesLR.write(degreesToUS(90));
    delay(11);
    eyesUD.write(degreesToUS(45));delay(245);eyesUD.write(degreesToUS(90));
    delay(215);
    mouth.write(degreesToUS(50));delay(141);mouth.write(degreesToUS(80));
    delay(861);
    eyesLR.write(degreesToUS(45));delay(194);eyesLR.write(degreesToUS(90));
    delay(31);
    eyesLR.write(degreesToUS(160));delay(316);eyesLR.write(degreesToUS(90));
    delay(42);
    eyesUD.write(degreesToUS(160));delay(173);eyesUD.write(degreesToUS(90));
    delay(10);
    eyesLR.write(degreesToUS(45));delay(102);eyesLR.write(degreesToUS(90));
    delay(12);
    eyesUD.write(degreesToUS(45));delay(244);eyesUD.write(degreesToUS(90));
    delay(11);
    eyesLR.write(degreesToUS(160));delay(173);eyesLR.write(degreesToUS(90));
    delay(11);
    eyesUD.write(degreesToUS(160));delay(41);eyesUD.write(degreesToUS(90));
    delay(349);
    eyesLR.write(degreesToUS(45));delay(235);eyesLR.write(degreesToUS(90));
    delay(30);
    eyesLR.write(degreesToUS(160));delay(418);eyesLR.write(degreesToUS(90));
    delay(41);
    eyesLR.write(degreesToUS(45));delay(195);eyesLR.write(degreesToUS(90));
    delay(20);
    eyesLR.write(degreesToUS(160));delay(255);eyesLR.write(degreesToUS(90));
    delay(41);
    eyesLR.write(degreesToUS(45));delay(184);eyesLR.write(degreesToUS(90));
    delay(21);
    eyesLR.write(degreesToUS(160));delay(397);eyesLR.write(degreesToUS(90));
    delay(22);
    eyesUD.write(degreesToUS(160));delay(234);eyesUD.write(degreesToUS(90));
    delay(10);
    eyesLR.write(degreesToUS(45));delay(31);eyesLR.write(degreesToUS(90));
    delay(11);
    eyesUD.write(degreesToUS(45));delay(458);eyesUD.write(degreesToUS(90));
    delay(11);
    eyesLR.write(degreesToUS(160));delay(204);eyesLR.write(degreesToUS(90));
    delay(42);
    eyesLR.write(degreesToUS(45));delay(214);eyesLR.write(degreesToUS(90));
    delay(10);
    eyesLR.write(degreesToUS(160));delay(255);eyesLR.write(degreesToUS(90));
    delay(12);
    eyesUD.write(degreesToUS(45));delay(142);eyesUD.write(degreesToUS(90));
    delay(11);
    eyesLR.write(degreesToUS(45));delay(275);eyesLR.write(degreesToUS(90));
    delay(31);
    eyesLR.write(degreesToUS(160));delay(408);eyesLR.write(degreesToUS(90));
    delay(51);
    eyesUD.write(degreesToUS(160));delay(265);eyesUD.write(degreesToUS(90));
    delay(605);
    mouth.write(degreesToUS(50));delay(222);mouth.write(degreesToUS(80));
    delay(421);
    eyesLR.write(degreesToUS(160));delay(183);eyesLR.write(degreesToUS(90));
    delay(42);
    eyesLR.write(degreesToUS(45));delay(163);eyesLR.write(degreesToUS(90));
    delay(30);
    eyesUD.write(degreesToUS(160));delay(622);eyesUD.write(degreesToUS(90));
    delay(1);
    eyesLR.write(degreesToUS(160));delay(10);eyesLR.write(degreesToUS(90));
    delay(0);
    eyesLR.write(degreesToUS(45));delay(52);eyesLR.write(degreesToUS(90));
    delay(11);
    eyesUD.write(degreesToUS(45));delay(245);eyesUD.write(degreesToUS(90));
    delay(10);
    eyesLR.write(degreesToUS(160));delay(428);eyesLR.write(degreesToUS(90));
    delay(42);
    eyesLR.write(degreesToUS(45));delay(214);eyesLR.write(degreesToUS(90));
    delay(10);
    eyesLR.write(degreesToUS(160));delay(479);eyesLR.write(degreesToUS(90));
    delay(62);
    eyesUD.write(degreesToUS(160));delay(377);eyesUD.write(degreesToUS(90));
    delay(11);
    eyesLR.write(degreesToUS(45));delay(20);eyesLR.write(degreesToUS(90));
    delay(31);
    eyesUD.write(degreesToUS(45));delay(428);eyesUD.write(degreesToUS(90));
    delay(11);
    eyesLR.write(degreesToUS(160));delay(478);eyesLR.write(degreesToUS(90));
    delay(42);
    eyesLR.write(degreesToUS(45));delay(448);eyesLR.write(degreesToUS(90));
    delay(32);
    eyesLR.write(degreesToUS(160));delay(457);eyesLR.write(degreesToUS(90));
    delay(32);
    eyesLR.write(degreesToUS(45));delay(489);eyesLR.write(degreesToUS(90));
    delay(11);
    eyesUD.write(degreesToUS(160));delay(397);eyesUD.write(degreesToUS(90));
    delay(10);
    eyesLR.write(degreesToUS(45));delay(32);eyesLR.write(degreesToUS(90));
    delay(10);
    eyesUD.write(degreesToUS(45));delay(439);eyesUD.write(degreesToUS(90));
    delay(10);
    eyesLR.write(degreesToUS(160));delay(255);eyesLR.write(degreesToUS(90));
    delay(288);
    mouth.write(degreesToUS(50));delay(120);mouth.write(degreesToUS(80));
    delay(94);
    mouth.write(degreesToUS(50));delay(271);mouth.write(degreesToUS(80));
    delay(1353);
    mouth.write(degreesToUS(50));delay(121);mouth.write(degreesToUS(80));
    delay(83);
    mouth.write(degreesToUS(50));delay(131);mouth.write(degreesToUS(80));
    delay(144);
    mouth.write(degreesToUS(50));delay(100);mouth.write(degreesToUS(80));
    delay(104);
    mouth.write(degreesToUS(50));delay(110);mouth.write(degreesToUS(80));
    delay(145);
    mouth.write(degreesToUS(50));delay(111);mouth.write(degreesToUS(80));
    delay(103);
    mouth.write(degreesToUS(50));delay(121);mouth.write(degreesToUS(80));
    delay(144);
    mouth.write(degreesToUS(50));delay(181);mouth.write(degreesToUS(80));
    delay(247);
    mouth.write(degreesToUS(50));delay(121);mouth.write(degreesToUS(80));
    delay(83);
    mouth.write(degreesToUS(50));delay(131);mouth.write(degreesToUS(80));
    delay(153);
    mouth.write(degreesToUS(50));delay(92);mouth.write(degreesToUS(80));
    delay(123);
    mouth.write(degreesToUS(50));delay(121);mouth.write(degreesToUS(80));
    delay(165);
    mouth.write(degreesToUS(50));delay(152);mouth.write(degreesToUS(80));
    delay(338);
    mouth.write(degreesToUS(50));delay(131);mouth.write(degreesToUS(80));
    delay(72);
    mouth.write(degreesToUS(50));delay(131);mouth.write(degreesToUS(80));
    delay(135);
    mouth.write(degreesToUS(50));delay(100);mouth.write(degreesToUS(80));
    delay(103);
    mouth.write(degreesToUS(50));delay(102);mouth.write(degreesToUS(80));
    delay(133);
    mouth.write(degreesToUS(50));delay(101);mouth.write(degreesToUS(80));
    delay(103);
    mouth.write(degreesToUS(50));delay(111);mouth.write(degreesToUS(80));
    delay(144);
    mouth.write(degreesToUS(50));delay(111);mouth.write(degreesToUS(80));
    delay(104);
    mouth.write(degreesToUS(50));delay(121);mouth.write(degreesToUS(80));
    delay(154);
    mouth.write(degreesToUS(50));delay(202);mouth.write(degreesToUS(80));
    delay(235);
    mouth.write(degreesToUS(50));delay(133);mouth.write(degreesToUS(80));
    delay(112);
    mouth.write(degreesToUS(50));delay(312);mouth.write(degreesToUS(80));
    delay(1353);
    mouth.write(degreesToUS(50));delay(121);mouth.write(degreesToUS(80));
    delay(73);
    mouth.write(degreesToUS(50));delay(141);mouth.write(degreesToUS(80));
    delay(103);
    mouth.write(degreesToUS(50));delay(111);mouth.write(degreesToUS(80));
    delay(124);
    mouth.write(degreesToUS(50));delay(111);mouth.write(degreesToUS(80));
    delay(123);
    mouth.write(degreesToUS(50));delay(121);mouth.write(degreesToUS(80));
    delay(124);
    mouth.write(degreesToUS(50));delay(112);mouth.write(degreesToUS(80));
    delay(153);
    mouth.write(degreesToUS(50));delay(142);mouth.write(degreesToUS(80));
    delay(308);
    mouth.write(degreesToUS(50));delay(121);mouth.write(degreesToUS(80));
    delay(83);
    mouth.write(degreesToUS(50));delay(121);mouth.write(degreesToUS(80));
    delay(113);
    mouth.write(degreesToUS(50));delay(131);mouth.write(degreesToUS(80));
    delay(104);
    mouth.write(degreesToUS(50));delay(110);mouth.write(degreesToUS(80));
    delay(165);
    mouth.write(degreesToUS(50));delay(192);mouth.write(degreesToUS(80));
    delay(267);
    mouth.write(degreesToUS(50));delay(112);mouth.write(degreesToUS(80));
    delay(92);
    mouth.write(degreesToUS(50));delay(122);mouth.write(degreesToUS(80));
    delay(143);
    mouth.write(degreesToUS(50));delay(122);mouth.write(degreesToUS(80));
    delay(123);
    mouth.write(degreesToUS(50));delay(121);mouth.write(degreesToUS(80));
    delay(144);
    mouth.write(degreesToUS(50));delay(101);mouth.write(degreesToUS(80));
    delay(114);
    mouth.write(degreesToUS(50));delay(100);mouth.write(degreesToUS(80));
    delay(124);
    mouth.write(degreesToUS(50));delay(102);mouth.write(degreesToUS(80));
    delay(112);
    mouth.write(degreesToUS(50));delay(91);mouth.write(degreesToUS(80));
    delay(124);
    mouth.write(degreesToUS(50));delay(192);mouth.write(degreesToUS(80));
    delay(267);
    mouth.write(degreesToUS(50));delay(121);mouth.write(degreesToUS(80));
    delay(124);
    mouth.write(degreesToUS(50));delay(343);mouth.write(degreesToUS(80));
    delay(1310);
    mouth.write(degreesToUS(50));delay(132);mouth.write(degreesToUS(80));
    delay(134);
    mouth.write(degreesToUS(50));delay(111);mouth.write(degreesToUS(80));
    delay(256);
    mouth.write(degreesToUS(50));delay(91);mouth.write(degreesToUS(80));
    delay(83);
    mouth.write(degreesToUS(50));delay(111);mouth.write(degreesToUS(80));
    delay(114);
    mouth.write(degreesToUS(50));delay(110);mouth.write(degreesToUS(80));
    delay(104);
    mouth.write(degreesToUS(50));delay(101);mouth.write(degreesToUS(80));
    delay(113);
    mouth.write(degreesToUS(50));delay(132);mouth.write(degreesToUS(80));
    delay(327);
    mouth.write(degreesToUS(50));delay(122);mouth.write(degreesToUS(80));
    delay(92);
    mouth.write(degreesToUS(50));delay(102);mouth.write(degreesToUS(80));
    delay(143);
    mouth.write(degreesToUS(50));delay(102);mouth.write(degreesToUS(80));
    delay(113);
    mouth.write(degreesToUS(50));delay(111);mouth.write(degreesToUS(80));
    delay(144);
    mouth.write(degreesToUS(50));delay(131);mouth.write(degreesToUS(80));
    delay(319);
    mouth.write(degreesToUS(50));delay(121);mouth.write(degreesToUS(80));
    delay(113);
    mouth.write(degreesToUS(50));delay(111);mouth.write(degreesToUS(80));
    delay(134);
    mouth.write(degreesToUS(50));delay(112);mouth.write(degreesToUS(80));
    delay(112);
    mouth.write(degreesToUS(50));delay(112);mouth.write(degreesToUS(80));
    delay(123);
    mouth.write(degreesToUS(50));delay(111);mouth.write(degreesToUS(80));
    delay(123);
    mouth.write(degreesToUS(50));delay(102);mouth.write(degreesToUS(80));
    delay(134);
    mouth.write(degreesToUS(50));delay(110);mouth.write(degreesToUS(80));
    delay(114);
    mouth.write(degreesToUS(50));delay(112);mouth.write(degreesToUS(80));
    delay(133);
    mouth.write(degreesToUS(50));delay(212);mouth.write(degreesToUS(80));
    delay(236);
    mouth.write(degreesToUS(50));delay(121);mouth.write(degreesToUS(80));
    delay(134);
    mouth.write(degreesToUS(50));delay(342);mouth.write(degreesToUS(80));
    delay(1251);
    mouth.write(degreesToUS(50));delay(151);mouth.write(degreesToUS(80));
    delay(92);
    mouth.write(degreesToUS(50));delay(143);mouth.write(degreesToUS(80));
    delay(122);
    mouth.write(degreesToUS(50));delay(112);mouth.write(degreesToUS(80));
    delay(92);
    mouth.write(degreesToUS(50));delay(122);mouth.write(degreesToUS(80));
    delay(133);
    mouth.write(degreesToUS(50));delay(122);mouth.write(degreesToUS(80));
    delay(113);
    mouth.write(degreesToUS(50));delay(122);mouth.write(degreesToUS(80));
    delay(133);
    mouth.write(degreesToUS(50));delay(141);mouth.write(degreesToUS(80));
    delay(298);
    mouth.write(degreesToUS(50));delay(121);mouth.write(degreesToUS(80));
    delay(93);
    mouth.write(degreesToUS(50));delay(112);mouth.write(degreesToUS(80));
    delay(143);
    mouth.write(degreesToUS(50));delay(122);mouth.write(degreesToUS(80));
    delay(112);
    mouth.write(degreesToUS(50));delay(122);mouth.write(degreesToUS(80));
    delay(164);
    mouth.write(degreesToUS(50));delay(273);mouth.write(degreesToUS(80));
    delay(185);
    mouth.write(degreesToUS(50));delay(151);mouth.write(degreesToUS(80));
    delay(82);
    mouth.write(degreesToUS(50));delay(122);mouth.write(degreesToUS(80));
    delay(113);
    mouth.write(degreesToUS(50));delay(122);mouth.write(degreesToUS(80));
    delay(112);
    mouth.write(degreesToUS(50));delay(122);mouth.write(degreesToUS(80));
    delay(123);
    mouth.write(degreesToUS(50));delay(675);mouth.write(degreesToUS(80));
    delay(318);
    mouth.write(degreesToUS(50));delay(142);mouth.write(degreesToUS(80));
    delay(72);
    mouth.write(degreesToUS(50));delay(121);mouth.write(degreesToUS(80));
    delay(114);
    mouth.write(degreesToUS(50));delay(110);mouth.write(degreesToUS(80));
    delay(104);
    mouth.write(degreesToUS(50));delay(483);mouth.write(degreesToUS(80));
    delay(646);
    mouth.write(degreesToUS(50));delay(131);mouth.write(degreesToUS(80));
    delay(73);
    mouth.write(degreesToUS(50));delay(121);mouth.write(degreesToUS(80));
    delay(134);
    mouth.write(degreesToUS(50));delay(715);mouth.write(degreesToUS(80));
    delay(492);
    mouth.write(degreesToUS(50));delay(131);mouth.write(degreesToUS(80));
    delay(73);
    mouth.write(degreesToUS(50));delay(131);mouth.write(degreesToUS(80));
    delay(104);
    mouth.write(degreesToUS(50));delay(121);mouth.write(degreesToUS(80));
    delay(123);
    mouth.write(degreesToUS(50));delay(131);mouth.write(degreesToUS(80));
    delay(123);
    mouth.write(degreesToUS(50));delay(485);mouth.write(degreesToUS(80));
    delay(317);
    mouth.write(degreesToUS(50));delay(122);mouth.write(degreesToUS(80));
    delay(123);
    mouth.write(degreesToUS(50));delay(132);mouth.write(degreesToUS(80));
    delay(113);
    mouth.write(degreesToUS(50));delay(141);mouth.write(degreesToUS(80));
    delay(103);
    mouth.write(degreesToUS(50));delay(685);mouth.write(degreesToUS(80));
    delay(483);
    mouth.write(degreesToUS(50));delay(131);mouth.write(degreesToUS(80));
    delay(92);
    mouth.write(degreesToUS(50));delay(132);mouth.write(degreesToUS(80));
    delay(113);
    mouth.write(degreesToUS(50));delay(132);mouth.write(degreesToUS(80));
    delay(143);
    mouth.write(degreesToUS(50));delay(112);mouth.write(degreesToUS(80));
    delay(92);
    mouth.write(degreesToUS(50));delay(524);mouth.write(degreesToUS(80));
    delay(360);
    mouth.write(degreesToUS(50));delay(131);mouth.write(degreesToUS(80));
    delay(144);
    mouth.write(degreesToUS(50));delay(131);mouth.write(degreesToUS(80));
    delay(123);
    mouth.write(degreesToUS(50));delay(122);mouth.write(degreesToUS(80));
    delay(133);
    mouth.write(degreesToUS(50));delay(1049);mouth.write(degreesToUS(80));
    delay(153);
    mouth.write(degreesToUS(50));delay(132);mouth.write(degreesToUS(80));
    delay(92);
    mouth.write(degreesToUS(50));delay(142);mouth.write(degreesToUS(80));
    delay(124);
    mouth.write(degreesToUS(50));delay(131);mouth.write(degreesToUS(80));
    delay(103);
    mouth.write(degreesToUS(50));delay(806);mouth.write(degreesToUS(80));
    delay(236);
    mouth.write(degreesToUS(50));delay(151);mouth.write(degreesToUS(80));
    delay(93);
    mouth.write(degreesToUS(50));delay(131);mouth.write(degreesToUS(80));
    delay(83);
    mouth.write(degreesToUS(50));delay(142);mouth.write(degreesToUS(80));
    delay(112);
    mouth.write(degreesToUS(50));delay(545);mouth.write(degreesToUS(80));
    delay(215);
    mouth.write(degreesToUS(50));delay(132);mouth.write(degreesToUS(80));
    delay(72);
    mouth.write(degreesToUS(50));delay(151);mouth.write(degreesToUS(80));
    delay(124);
    mouth.write(degreesToUS(50));delay(473);mouth.write(degreesToUS(80));
    delay(247);
    mouth.write(degreesToUS(50));delay(131);mouth.write(degreesToUS(80));
    delay(104);
    mouth.write(degreesToUS(50));delay(121);mouth.write(degreesToUS(80));
    delay(113);
    mouth.write(degreesToUS(50));delay(131);mouth.write(degreesToUS(80));
    delay(114);
    mouth.write(degreesToUS(50));delay(131);mouth.write(degreesToUS(80));
    delay(92);
    mouth.write(degreesToUS(50));delay(132);mouth.write(degreesToUS(80));
    delay(82);
    mouth.write(degreesToUS(50));delay(132);mouth.write(degreesToUS(80));
    delay(113);
    mouth.write(degreesToUS(50));delay(283);mouth.write(degreesToUS(80));
    delay(174);
    mouth.write(degreesToUS(50));delay(132);mouth.write(degreesToUS(80));
    delay(92);
    mouth.write(degreesToUS(50));delay(141);mouth.write(degreesToUS(80));
    delay(124);
    mouth.write(degreesToUS(50));delay(141);mouth.write(degreesToUS(80));
    delay(114);
    mouth.write(degreesToUS(50));delay(141);mouth.write(degreesToUS(80));
    delay(123);
    mouth.write(degreesToUS(50));delay(122);mouth.write(degreesToUS(80));
    delay(103);
    mouth.write(degreesToUS(50));delay(132);mouth.write(degreesToUS(80));
    delay(112);
    mouth.write(degreesToUS(50));delay(202);mouth.write(degreesToUS(80));
    delay(226);
    mouth.write(degreesToUS(50));delay(152);mouth.write(degreesToUS(80));
    delay(72);
    mouth.write(degreesToUS(50));delay(132);mouth.write(degreesToUS(80));
    delay(112);
    mouth.write(degreesToUS(50));delay(122);mouth.write(degreesToUS(80));
    delay(102);
    mouth.write(degreesToUS(50));delay(142);mouth.write(degreesToUS(80));
    delay(247);
    eyesLR.write(degreesToUS(45));delay(560);eyesLR.write(degreesToUS(90));
    delay(10);
    eyesLR.write(degreesToUS(160));delay(723);eyesLR.write(degreesToUS(90));
    delay(11);
    eyesLR.write(degreesToUS(45));delay(397);eyesLR.write(degreesToUS(90));
    delay(22);
    eyesUD.write(degreesToUS(160));delay(204);eyesUD.write(degreesToUS(90));
    delay(256);
    eyesLR.write(degreesToUS(160));delay(234);eyesLR.write(degreesToUS(90));
    delay(52);
    eyesLR.write(degreesToUS(45));delay(316);eyesLR.write(degreesToUS(90));
    delay(11);
    eyesUD.write(degreesToUS(160));delay(153);eyesUD.write(degreesToUS(90));
    delay(83);
    eyesUD.write(degreesToUS(45));delay(82);eyesUD.write(degreesToUS(90));
    delay(10);
    eyesLR.write(degreesToUS(160));delay(174);eyesLR.write(degreesToUS(90));
    delay(51);
    mouth.write(degreesToUS(50));delay(162);mouth.write(degreesToUS(80));
    delay(103);
    mouth.write(degreesToUS(50));delay(142);mouth.write(degreesToUS(80));
    delay(71);
    mouth.write(degreesToUS(50));delay(263);mouth.write(degreesToUS(80));
    delay(902);
    eyesLR.write(degreesToUS(45));delay(296);eyesLR.write(degreesToUS(90));
    delay(174);
    mouth.write(degreesToUS(50));delay(121);mouth.write(degreesToUS(80));
    delay(93);
    mouth.write(degreesToUS(50));delay(132);mouth.write(degreesToUS(80));
    delay(133);
    mouth.write(degreesToUS(50));delay(132);mouth.write(degreesToUS(80));
    delay(92);
    mouth.write(degreesToUS(50));delay(131);mouth.write(degreesToUS(80));
    delay(134);
    mouth.write(degreesToUS(50));delay(121);mouth.write(degreesToUS(80));
    delay(114);
    mouth.write(degreesToUS(50));delay(121);mouth.write(degreesToUS(80));
    delay(113);
    mouth.write(degreesToUS(50));delay(152);mouth.write(degreesToUS(80));
    delay(307);
    mouth.write(degreesToUS(50));delay(142);mouth.write(degreesToUS(80));
    delay(164);
    mouth.write(degreesToUS(50));delay(122);mouth.write(degreesToUS(80));
    delay(92);
    mouth.write(degreesToUS(50));delay(122);mouth.write(degreesToUS(80));
    delay(103);
    mouth.write(degreesToUS(50));delay(121);mouth.write(degreesToUS(80));
    delay(133);
    mouth.write(degreesToUS(50));delay(152);mouth.write(degreesToUS(80));
    delay(790);
    eyesLR.write(degreesToUS(160));delay(397);eyesLR.write(degreesToUS(90));
    delay(51);
    eyesLR.write(degreesToUS(45));delay(397);eyesLR.write(degreesToUS(90));
    delay(32);
    eyesUD.write(degreesToUS(160));delay(326);eyesUD.write(degreesToUS(90));
    delay(1);
    eyesLR.write(degreesToUS(160));delay(10);eyesLR.write(degreesToUS(90));
    delay(124);
    mouth.write(degreesToUS(50));delay(302);mouth.write(degreesToUS(80));
    delay(165);
    mouth.write(degreesToUS(50));delay(141);mouth.write(degreesToUS(80));
    delay(92);
    mouth.write(degreesToUS(50));delay(364);mouth.write(degreesToUS(80));
    delay(399);
    eyesUD.write(degreesToUS(45));delay(387);eyesUD.write(degreesToUS(90));
    delay(493);
    mouth.write(degreesToUS(50));delay(122);mouth.write(degreesToUS(80));
    delay(92);
    mouth.write(degreesToUS(50));delay(111);mouth.write(degreesToUS(80));
    delay(144);
    mouth.write(degreesToUS(50));delay(132);mouth.write(degreesToUS(80));
    delay(113);
    mouth.write(degreesToUS(50));delay(131);mouth.write(degreesToUS(80));
    delay(134);
    mouth.write(degreesToUS(50));delay(131);mouth.write(degreesToUS(80));
    delay(113);
    mouth.write(degreesToUS(50));delay(121);mouth.write(degreesToUS(80));
    delay(104);
    mouth.write(degreesToUS(50));delay(162);mouth.write(degreesToUS(80));
    delay(450);
    mouth.write(degreesToUS(50));delay(131);mouth.write(degreesToUS(80));
    delay(52);
    mouth.write(degreesToUS(50));delay(131);mouth.write(degreesToUS(80));
    delay(63);
    mouth.write(degreesToUS(50));delay(111);mouth.write(degreesToUS(80));
    delay(72);
    mouth.write(degreesToUS(50));delay(131);mouth.write(degreesToUS(80));
    delay(83);
    mouth.write(degreesToUS(50));delay(212);mouth.write(degreesToUS(80));
    delay(256);
    eyesLR.write(degreesToUS(45));delay(449);eyesLR.write(degreesToUS(90));
    delay(52);
    eyesLR.write(degreesToUS(160));delay(377);eyesLR.write(degreesToUS(90));
    delay(10);
    eyesUD.write(degreesToUS(160));delay(438);eyesUD.write(degreesToUS(90));
    delay(42);
    eyesUD.write(degreesToUS(45));delay(357);eyesUD.write(degreesToUS(90));
    delay(143);
    mouth.write(degreesToUS(50));delay(182);mouth.write(degreesToUS(80));
    delay(297);
    mouth.write(degreesToUS(50));delay(152);mouth.write(degreesToUS(80));
    delay(83);
    mouth.write(degreesToUS(50));delay(403);mouth.write(degreesToUS(80));
    delay(1280);
    mouth.write(degreesToUS(50));delay(141);mouth.write(degreesToUS(80));
    delay(83);
    mouth.write(degreesToUS(50));delay(121);mouth.write(degreesToUS(80));
    delay(104);
    mouth.write(degreesToUS(50));delay(121);mouth.write(degreesToUS(80));
    delay(113);
    mouth.write(degreesToUS(50));delay(102);mouth.write(degreesToUS(80));
    delay(133);
    mouth.write(degreesToUS(50));delay(132);mouth.write(degreesToUS(80));
    delay(92);
    mouth.write(degreesToUS(50));delay(132);mouth.write(degreesToUS(80));
    delay(123);
    mouth.write(degreesToUS(50));delay(162);mouth.write(degreesToUS(80));
    delay(307);
    mouth.write(degreesToUS(50));delay(112);mouth.write(degreesToUS(80));
    delay(72);
    mouth.write(degreesToUS(50));delay(131);mouth.write(degreesToUS(80));
    delay(123);
    mouth.write(degreesToUS(50));delay(122);mouth.write(degreesToUS(80));
    delay(113);
    mouth.write(degreesToUS(50));delay(132);mouth.write(degreesToUS(80));
    delay(143);
    mouth.write(degreesToUS(50));delay(162);mouth.write(degreesToUS(80));
    delay(288);
    mouth.write(degreesToUS(50));delay(120);mouth.write(degreesToUS(80));
    delay(94);
    mouth.write(degreesToUS(50));delay(121);mouth.write(degreesToUS(80));
    delay(123);
    mouth.write(degreesToUS(50));delay(121);mouth.write(degreesToUS(80));
    delay(104);
    mouth.write(degreesToUS(50));delay(131);mouth.write(degreesToUS(80));
    delay(133);
    mouth.write(degreesToUS(50));delay(474);mouth.write(degreesToUS(80));
    //delay(493);        
    // Serial.print("done");
        
    }
 
}





