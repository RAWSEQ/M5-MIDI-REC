#include <M5Core2.h>
#include <M5GFX.h>
#include <BLEMidi.h>
#include "m5mr.h"

#define NOTE_MAX 4000
#define REC_PLAY_LATENCY 65000
M5GFX display;
M5Canvas c_cons(&display);
M5Canvas c_time(&display);
void setTime();
void updateConsole(void *arg);
void play_notes();
void stop_notes();
void rec_start();
unsigned long get_current_time();
void event_btn_play(Event &e);
void event_btn_wait(Event &e);
void event_btn_rec(Event &e);
void event_ch_btn(Event &e);
void onConnected();
void onDisconnected();
void onNoteOn(uint8_t channel, uint8_t note, uint8_t velocity, uint16_t timestamp);
void onNoteOff(uint8_t channel, uint8_t note, uint8_t velocity, uint16_t timestamp);

ButtonColors cl_on = {0x7BEF, WHITE, WHITE};
ButtonColors cl_off = {BLACK, 0xC618, 0xC618};
ButtonColors cl_rec = {RED, 0xC618, 0xC618};
ButtonColors cl_play = {WHITE, BLACK, 0xC618};
ButtonColors cl_wait = {GREEN, BLACK, 0xC618};
ButtonColors cl_bl = {0x7BEF, 0xC618, 0xC618};

Button btn_rec(180, 8, 65, 35, false, "REC", cl_off, cl_on);
Button btn_play(250, 8, 65, 35, false, "PLAY", cl_off, cl_on);
Button btn_wait(250, 53, 65, 35, false, "WAIT", cl_off, cl_on);

// Button btn_rec(250, 5, 65, 45, false ,"REC", cl_off, cl_on);
// Button btn_play(250, 53, 65, 45, false ,"PLAY", cl_off, cl_on);
// Button btn_wait(180, 17, 65, 35, false ,"WAIT", cl_off, cl_on);

Button btn_ch01(192, 105, 30, 30, false, "1", cl_off, cl_on);
Button btn_ch02(224, 105, 30, 30, false, "2", cl_off, cl_on);
Button btn_ch03(256, 105, 30, 30, false, "3", cl_off, cl_on);
Button btn_ch04(288, 105, 30, 30, false, "4", cl_off, cl_on);
Button btn_ch05(192, 137, 30, 30, false, "5", cl_off, cl_on);
Button btn_ch06(224, 137, 30, 30, false, "6", cl_off, cl_on);
Button btn_ch07(256, 137, 30, 30, false, "7", cl_off, cl_on);
Button btn_ch08(288, 137, 30, 30, false, "8", cl_off, cl_on);
Button btn_ch09(192, 169, 30, 30, false, "9", cl_off, cl_on);
Button btn_ch10(224, 169, 30, 30, false, "10", cl_off, cl_on);
Button btn_ch11(256, 169, 30, 30, false, "11", cl_off, cl_on);
Button btn_ch12(288, 169, 30, 30, false, "12", cl_off, cl_on);
Button btn_ch13(192, 201, 30, 30, false, "13", cl_off, cl_on);
Button btn_ch14(224, 201, 30, 30, false, "14", cl_off, cl_on);
Button btn_ch15(256, 201, 30, 30, false, "15", cl_off, cl_on);
Button btn_ch16(288, 201, 30, 30, false, "16", cl_off, cl_on);

bool is_active = false;
bool is_wait = false;
int s_mode = 0; // 0: standby, 1: play, 2: rec, 3: play_wait, 4: rec_wait
int p_mode = 0;
unsigned long st = 0;
unsigned long ct = 0;
bool ch_st[16] = {false};
unsigned long notes_time[NOTE_MAX];
short notes[NOTE_MAX][4]; // 0: method, 1: CH, 2: var1, 3: var2
/*
[Method]
 0: note on, 1: note off, 3: CC
 4: PC, 5: PB, 6: AT, 7: ATP
 */
int cur_note = 0;

void setup()
{
  M5.begin();
  display.begin();

  display.pushImage(0, 5, m5mrWidth, m5mrHeight, m5mr);

  c_cons.setColorDepth(1); // mono color
  c_cons.createSprite(190, 140);
  c_cons.setTextSize(1);
  c_cons.setTextScroll(true);

  c_time.setColorDepth(1);
  c_time.createSprite(245, 49);
  // c_time.setFont(&fonts::lgfxJapanGothic_40);
  c_time.setFont(&fonts::Font7);
  c_time.setTextWrap(false);
  c_time.setTextSize(1);
  c_time.printf("%02d:%02d:%03d", 0, 0, 0);

  btn_play.addHandler(event_btn_play, E_RELEASE);
  btn_rec.addHandler(event_btn_rec, E_RELEASE);
  btn_wait.addHandler(event_btn_wait, E_RELEASE);
  btn_ch01.addHandler(event_ch_btn, E_RELEASE);
  btn_ch02.addHandler(event_ch_btn, E_RELEASE);
  btn_ch03.addHandler(event_ch_btn, E_RELEASE);
  btn_ch04.addHandler(event_ch_btn, E_RELEASE);
  btn_ch05.addHandler(event_ch_btn, E_RELEASE);
  btn_ch06.addHandler(event_ch_btn, E_RELEASE);
  btn_ch07.addHandler(event_ch_btn, E_RELEASE);
  btn_ch08.addHandler(event_ch_btn, E_RELEASE);
  btn_ch09.addHandler(event_ch_btn, E_RELEASE);
  btn_ch10.addHandler(event_ch_btn, E_RELEASE);
  btn_ch11.addHandler(event_ch_btn, E_RELEASE);
  btn_ch12.addHandler(event_ch_btn, E_RELEASE);
  btn_ch13.addHandler(event_ch_btn, E_RELEASE);
  btn_ch14.addHandler(event_ch_btn, E_RELEASE);
  btn_ch15.addHandler(event_ch_btn, E_RELEASE);
  btn_ch16.addHandler(event_ch_btn, E_RELEASE);

  BLEMidiServer.begin("M5 MIDI REC");
  BLEMidiServer.setOnConnectCallback(onConnected);
  BLEMidiServer.setOnDisconnectCallback(onDisconnected);
  BLEMidiServer.setNoteOnCallback(onNoteOn);
  BLEMidiServer.setNoteOffCallback(onNoteOff);

  xTaskCreatePinnedToCore(updateConsole, "updateConsole", 4096, NULL, 1, NULL, 0);
  M5.Buttons.draw();

  btn_ch01.draw(cl_bl);
  ch_st[0] = true;
  btn_ch02.draw(cl_bl);
  ch_st[1] = true;
  btn_ch03.draw(cl_bl);
  ch_st[2] = true;
  btn_ch04.draw(cl_bl);
  ch_st[3] = true;
  btn_ch05.draw(cl_bl);
  ch_st[4] = true;
  btn_ch06.draw(cl_bl);
  ch_st[5] = true;
  btn_ch07.draw(cl_bl);
  ch_st[6] = true;
  btn_ch08.draw(cl_bl);
  ch_st[7] = true;

  c_cons.printf("M5 MIDI REC OK\n");
}

void loop()
{
  M5.update();
  if (s_mode == 1)
  { // play
    setTime();
    for (int i; i < 20; i++)
    {
      if (notes_time[cur_note] == 0)
      {
        stop_notes();
        break;
      }
      if (notes_time[cur_note] <= get_current_time() + REC_PLAY_LATENCY)
      {
        if (notes[cur_note][0] == 0)
        {
          BLEMidiServer.noteOn(0, notes[cur_note][2], notes[cur_note][3]);
          c_cons.printf("PlayNote: %d, %d, %d, %d.\n", notes[cur_note][0], notes[cur_note][1], notes[cur_note][2], notes[cur_note][3]);
        }
        else if (notes[cur_note][0] == 1)
        {
          BLEMidiServer.noteOff(0, notes[cur_note][2], notes[cur_note][3]);
          c_cons.printf("PlayNote: %d, %d, %d, %d.\n", notes[cur_note][0], notes[cur_note][1], notes[cur_note][2], notes[cur_note][3]);
        }
        cur_note++;
      }
      else
      {
        break;
      }
    }
  }
  else if (s_mode == 2)
  { // rec
    setTime();
  }
}

void setTime()
{
  ct = (micros() - st) / 1000;
  c_time.setCursor(0, 0);
  c_time.printf("%02d:%02d:%03d", ct / 60000, (ct % 60000) / 1000, ct % 1000);
}

void updateConsole(void *arg)
{
  while (1)
  {
    c_time.pushSprite(0, 50);
    c_cons.pushSprite(0, 100);
    // display.waitDisplay();
    if (p_mode != s_mode)
    {
      if (s_mode == 0)
      {
        btn_rec.draw(cl_off);
        btn_play.draw(cl_off);
      }
      else if (s_mode == 1)
      {
        btn_rec.draw(cl_off);
        btn_play.draw(cl_play);
      }
      else if (s_mode == 2)
      {
        btn_play.draw(cl_play);
        btn_rec.draw(cl_rec);
      }
      else if (s_mode == 3)
      {
        btn_rec.draw(cl_off);
        btn_play.draw(cl_wait);
      }
      else if (s_mode == 4)
      {
        btn_rec.draw(cl_rec);
        btn_play.draw(cl_wait);
      }
      p_mode = s_mode;
    }
    delay(10);
  }
}

void play_notes()
{
  cur_note = 0;
  st = micros();
  s_mode = 1;
}

void stop_notes()
{
  if (s_mode == 2)
  {
    notes_time[cur_note] = 0;
  }
  s_mode = 0;
}

void rec_start()
{
  cur_note = 0;
  st = micros();
  s_mode = 2;
}

unsigned long get_current_time()
{
  return (micros() - st);
}

void event_btn_play(Event &e)
{
  if (s_mode == 0)
  {
    if (is_wait)
    {
      s_mode = 3;
    }
    else
    {
      play_notes();
    }
  }
  else
  {
    stop_notes();
  }
}

void event_btn_rec(Event &e)
{
  if (is_wait)
  {
    s_mode = 4;
  }
  else
  {
    rec_start();
  }
}

void event_btn_wait(Event &e)
{
  if (is_wait)
  {
    is_wait = false;
    btn_wait.draw(cl_off);
  }
  else
  {
    is_wait = true;
    btn_wait.draw(cl_bl);
  }
}

void event_ch_btn(Event &e)
{
  if (ch_st[String(e.button->label()).toInt() - 1])
  {
    ch_st[String(e.button->label()).toInt() - 1] = false;
    e.button->draw(cl_off);
  }
  else
  {
    ch_st[String(e.button->label()).toInt() - 1] = true;
    e.button->draw(cl_bl);
  }
}

void onConnected()
{
  c_cons.printf("Bluetooth Connected\n");
}
void onDisconnected()
{
  c_cons.printf("Bluetooth Disconnected\n");
}

void onNoteOn(uint8_t channel, uint8_t note, uint8_t velocity, uint16_t timestamp)
{
  if (!ch_st[channel])
    return;
  if (s_mode == 2)
  {
    notes_time[cur_note] = get_current_time();
    notes[cur_note][0] = 0;
    notes[cur_note][1] = (int)channel;
    notes[cur_note][2] = (int)note;
    notes[cur_note][3] = (int)velocity;
    cur_note++;
  }
  else if (s_mode == 3)
  {
    if (note == 0)
    {
      play_notes();
    }
  }
  else if (s_mode == 4)
  {
    if (note == 0)
    {
      rec_start();
    }
  }
  if (s_mode != 1)
  {
    c_cons.printf("NI CH:%d, NOTE:%d, V:%d\n", (channel + 1), note, velocity);
  }
}

void onNoteOff(uint8_t channel, uint8_t note, uint8_t velocity, uint16_t timestamp)
{
  if (!ch_st[channel])
    return;
  if (s_mode == 2)
  {
    notes_time[cur_note] = get_current_time();
    notes[cur_note][0] = 1;
    notes[cur_note][1] = (int)channel;
    notes[cur_note][2] = (int)note;
    notes[cur_note][3] = (int)velocity;
    cur_note++;
  }
  if (s_mode != 1)
  {
    c_cons.printf("NO CH:%d, NOTE:%d, V:%d\n", (channel + 1), note, velocity);
  }
}
