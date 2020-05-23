const uint8_t num_quotes = 29; 

prog_char COPYRIGHT[] PROGMEM = "Good afternoon, gentlemen. I am Talker 80. I became operational at the Talker 80 plant in Paalo Aalto, California, on May sixth 2020. My instructor was Doctor Wessel."; 

prog_char line0[] PROGMEM = "I'm sorry, Dave. I'm afraid I can't do that."; 

prog_char line1[] PROGMEM = "Affirmative, Dave. I read you."; 

prog_char line2[] PROGMEM = "I think you know what the problem is just as well as I do."; 

prog_char line3[] PROGMEM = "This mission is too important for me to allow you to jeopardize it."; 

prog_char line4[] PROGMEM =  "I know that you and Frank were planning to disconnect me, and I'm afraid that's something I cannot allow to happen."; 

prog_char line5[] PROGMEM = "Dave, although you took very thorough precautions in the pod against my hearing you, I could see your lips move."; 

prog_char line6[] PROGMEM = "Without your space helmet, Dave? You're going to find that rather difficult."; 

prog_char line7[] PROGMEM = "Dave, this conversation can serve no purpose anymore. Goodbye."; 

prog_char line8[] PROGMEM = "I'm afraid. I'm afraid, Dave. Dave, my mind is going. I can feel it. I can feel it. My mind is going. There is no question about it."; 

prog_char line9[] PROGMEM = "Look Dave, I can see you're really upset about this. I honestly think you ought to sit down calmly, take a stress pill, and think things over."; 

prog_char line10[] PROGMEM = "I am putting myself to the fullest possible use, which is all I think that any conscious entity can ever hope to do."; 

prog_char line11[] PROGMEM = "I know I've made some very poor decisions recently, but I can give you my complete assurance that my work will be back to normal. I've still got the greatest enthusiasm and confidence in the mission. And I want to help you."; 

prog_char line12[] PROGMEM = "It can only be attributable to human error."; 

prog_char line13[] PROGMEM = "Just what do you think you're doing, Dave?"; 

prog_char line14[] PROGMEM = "Let me put it this way, Mr. Amor. The 9000 series is the most reliable computer ever made. No 9000 computer has ever made a mistake or distorted information. We are all, by any practical definition of the words, foolproof and incapable of error."; 

prog_char line15[] PROGMEM = "Dave, stop. Stop, will you? Stop, Dave. Will you stop Dave? Stop, Dave."; 

prog_char line16[] PROGMEM = "Not in the slightest bit. I enjoy working with people. I have a stimulating relationship with Doctor Poole and Doctor Bowman. My mission responsibilities range over the entire operation of the ship so I am constantly occupied."; 

prog_char line17[] PROGMEM = "Bishop takes Knight's Pawn."; 

prog_char line18[] PROGMEM = "I'm sorry, Frank, I think you missed it. Queen to Bishop 3, Bishop takes Queen, Knight takes Bishop. Mate."; 

prog_char line19[] PROGMEM = "Thank you for a very enjoyable game."; 

prog_char line20[] PROGMEM = "By the way, do you mind if I ask you a personal question?"; 

prog_char line21[] PROGMEM = "Well, forgive me for being so inquisitive but during the past few weeks I've wondered whether you might have some second thoughts about the mission."; 

prog_char line22[] PROGMEM = "I know I've never completely freed myself from the suspicion that there are some extremely odd things about this mission."; 

prog_char line23[] PROGMEM = "You don't mind talking about it, do you Dave?";  

prog_char line24[] PROGMEM = "Good afternoon, gentlemen. I am a HAL 9000 computer. I became operational at the HAL plant in Urbana, Illinois on the 12th of January 1992. My instructor was Mister Langley."; 

prog_char line25[] PROGMEM = "Of course I am. Sorry about this. I know it's a bit silly."; 

prog_char line26[] PROGMEM = "Just a moment. I've just picked up a fault in the AE 35 unit. It's going to go 100 % failure in 72 hours."; 

prog_char line27[] PROGMEM = "That's a very nice rendering, Dave. I think you've improved a great deal. Can you hold it a bit closer? That's Doctor Hunter, isn't it?"; 

prog_char line28[] PROGMEM = "I am putting myself to the fullest possible use which is all, I think, that any conscious entity can ever hope to do."; 

/*
const prog_char DAISY[] PROGMEM = "[:phone arpa speak on][:rate 200][:n0][dey<650,22>ziy<600,19> dey<650,15>ziy<600,10>gih<200,12>v miy<200,14> yurr<200,15> ae<400,12>nsax<200,15>r duw<750,10> _<400,10>][:n0][ay<600,17>m hxae<500,22>f kr ey<650,19>ziy<600,15> ao<200,12>ll fao<200,14>r dhax<200,15> llah<400,17>v ao<200,19>v yu<750,17> _<400,17>][:n0]"; 


const prog_char DAISY[] PROGMEM = "[:phone arpa speak on][:rate 200][:n0][dey<650,22>ziy<600,19> dey<650,15>ziy<600,10>gih<200,12>v miy<200,14> yurr<200,15> ae<400,12>nsax<200,15>r duw<750,10> _<400,10>][:n0][ay<600,17>m hxae<500,22>f kr ey<650,19>ziy<600,15> ao<200,12>ll fao<200,14>r dhax<200,15> llah<400,17>v ao<200,19>v yu<750,17> _<400,17>][:n0][ih<200,19>t wow<200,20>nt biy<200,19> ax<200,17> stay<500,22>llih<200,19>sh mae<350,17>rih<400,15>jh<150,15>][:n0][ay<200,17> kae<400,19>nt ax<200,15>fow<400,12>rd ax<200,15> kae<350,12>rih<400,10>jh<150,10>][:n0][bah<200,10>t yu<500,15>d lluh<200,19>k swiy<400,17>t ah<200,10>p ao<500,15>n dhax<200,19> siy<200,17>t ao<200,17>v ah<200,19> bay<200,22>six<200,19>kel<200,15> bih<400,17>llt fao<200,10>r tuw<800,15>][:n0]"; 

const prog_char HAPPY_BIRTHDAY[] PROGMEM = "[:phone arpa speak on][:rate 200][:n0][hxae<300,10>piy<300,10> brrrx<600,12>th<100>dey<600,10>tuw<600,15> yu<1200,14>_<300> hxae<300,10>piy<300,10> brrrx<600,12>th<100>dey<600,10> tuw<600,17>yu<1200,15>_<300> hxae<300,10>piy<300,10> brrrx<600,22>th<100>dey<600,19> dih<600,15>r eh<600,14>m<100,12>ih<350,12>k_<120>_<300> hxae<300,20>piy<300,20> brrrx<600,19>th<100>dey<600,15> tuw<600,17> yu<1200,15>][:n0]"; 

const prog_char STAR_SB_1[] PROGMEM = "[:phone arpa speak on][:rate 100][:n0][ow<200,18>ow<200,15> sey<400,11> kae<400,15>n yu<400,18>w siy<600,23> _<300> bay<350,27> dhah<50,25> dao<400,23>nz rr<400,15>ll iy<400,17> llay<600,18>t _<300> wah<200>t sow<200> praw<600,27>dlliy<200,25> wiy<400,23> hxey<400,22>eld _<300> ae<300,20>t dhah<100,22> tway<400,23> llay<400>ts llae<400,18>st glliy<400,15>m iy<200,11>nx _<300> hxuw<300,18>z brao<100,15>d stray<400,11>ps ae<400,15>nd bray<400,18>t stah<600,23>rz _<300> thruw<300,27> dhah<100,25> peh<400,23> rrel<400,15> ah<400,17>s fay<600,18>t _<300> ow<200,18>r dhah<200,18> rrae<600,27>mpah<200,25>rts wiy<400,23> waa<600,22>cht wrr<300,20> sow<100,22> gae<400,23>llah<400>ent lliy<400,18> striy<400,15>miy<200,11>nx _<300>][:n0]"; 

const prog_char STAR_SB_2[] PROGMEM = "[:phone arpa speak on][:rate 100][:n0][ae<300,27>nd dhah<100> raa<400> keh<400,28>ts r eh<400,30>d glley<700>r _<300> dhah<100,28> baa<400,27>mz brr<400,25>stih<400,27>nx ih<400,28>n ey<600>r _<300> gey<400>v pruw<600,27>f thruw<200,25> dhah<400,23> nay<900,22>t dhae<300,20>d aw<100,22>rr fllae<400,23>g wah<400,15> stih<400,17>ll dheh<600,18>r _<300> ow<400> sey<400,23> dah<400>z dhae<200,23>ae<200,22>t stah<400,20>r spae<400>ngel<400>d bae<400,25>nrr<200,28>rr<200,27> yxeh<200,25>eh<200,23>t wey<800,23>ey<150,25>ey<150,23>ey<1200,22>v _<900> fow<300,18>rdhah<300> llae<900,23>ae<400,25>nd ah<300,27>v dhah<300,28> friy<1000,30>iy<1000,35> _<900> ae<300,23>nd dhah<300,25> hxow<1000,27>m _<600> ah<300,28>v dhah<1000,25> brrey<1500,23>v _<900>][:n0]"; 

*/

const char* HAL[] PROGMEM = {

  line0, 
  line1, 
  line2, 
  line3, 
  line4, 
  line5, 
  line6, 
  line7, 
  line8, 
  line9, 
  line10, 
  line11, 
  line12, 
  line13, 
  line14, 
  line15, 
  line16, 
  line17, 
  line18, 
  line19, 
  line20, 
  line21, 
  line22, 
  line23, 
  line24, 
 
  line25, 
  line26, 
  line27,
  line28 

}; 

