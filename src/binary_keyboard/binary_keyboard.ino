#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Keyboard.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32

#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define INPUT_PIN 4

const int DIT_LENGTH[2] = {25, 200};
const int CHAR_LENGTH[2] = {20 , 1000};
const int WORD_LENGTH[2] = {2000, 3000};
const int DAH_LENGTH[2] = {201, 1000};

bool keyboardEnabled = true;
bool inputPressed = false;
bool processedUnpressed = true;
bool processedWaitChar = true;
bool processedWaitWord = true;

unsigned long startPressed = 0;
unsigned long endPressed = 0;
unsigned long endWait = 0;
unsigned long startWait = 0;

unsigned long waitTime = 0;
unsigned long pressedTime = 0;

bool shouldRedraw = true;

unsigned int currentLetter = 0;
int letterIndex = 0;

String displayString = "";

enum SPECIAL_KEYS {
  CTRL,
  SHIFT,
  ALT
};

const int KEYMAP[6][64] = {
  // 1 bit maps
  {
    (int)'e', (int)'t'
  },
  // 2 bit maps
  {
    (int) 'i', (int) 'a', (int) 'n', (int) 'm'
  },
  // 3 bit maps
  {
    (int) 's', (int) 'u', (int) 'r', (int) 'w',
    (int) 'd', (int) 'k', (int) 'g', (int) 'o'
  },
  // 4 bit maps
  {
    (int) 'h', (int) 'v', (int) 'f', -1,
    (int) 'l', -1, (int) 'p', (int) 'j',
    (int) 'b', (int) 'x', (int) 'c', (int) 'y',
    (int) 'z', (int) 'q'
  },
  // 5 bit maps
  {
    // 5, 4, ., 3, .
    53, 52, -1, 51, -1,
    // ., 2, ., ., +
    -1, 50, -1, -1, (int)'+',
    // ., ., ., ., .
    -1, -1, -1, -1, 49,
    // 6, =, /, ., .
    54, (int)'=', (int)'/', -1, -1,
    // ., ., ., 7, .,
    -1, -1, -1, 55, -1,
    // ., ., 8, ., 9
    -1, -1, 56, -1, 57,
    // 0
    48
  },
  // 6 bit maps
  {
    -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, (int) '`', -1, -1, (int) '?',
    -1, -1, -1, -1, -1, -1, -1,
    (int) '-', -1, -1, -1, -1, -1, -1,
    -1, (int) ';', (int) '!', -1, -1, -1, -1,
    -1, (int) ',', -1, -1, -1, -1, (int)':',
    -1, -1, -1, -1, -1, -1, -1,
  }
};

/**
 * Much similar to the binary keyboard, this code is hacky garbage
 * good luck and have fun
 */
void setup() {
  Serial.begin(9600);

  pinMode(INPUT_PIN, INPUT_PULLUP);

  Keyboard.begin();


  // Generate display voltage from 3.3 internally - from SSD1306 example
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("Display failed to load"));
    for (;;); // go into permanenet loop so we don't break stuff 
  }

}

void loop() {
  check_click();
  process_dits_and_dahs();
  draw();
}

bool check_click() {
  if (inputPressed) {
    pressedTime = millis() - startPressed;
    waitTime = 0;
  } else {
    waitTime = millis() - startWait;
    pressedTime = 0;
  }

  if (inputPressed && digitalRead(INPUT_PIN) == HIGH) {
    processedUnpressed = false;
    processedWaitChar = false;
    processedWaitWord = false;
    inputPressed = false;
    endPressed = millis();
    startWait = millis();
    print_click_status("not clicked");
  } else if (!inputPressed && digitalRead(INPUT_PIN) == LOW) {
    inputPressed = true;
    startPressed = millis();
    endWait = millis();

    processedWaitChar = true;
    processedWaitWord = true;
    print_click_status("clicked");
  }
}

void process_dits_and_dahs() {
  if (!processedWaitChar && isChar(waitTime)) {
    // send char
    send_char();
    processedWaitChar = true;
  }

  if (!processedWaitWord && isWord(waitTime)) {
    // send word
    send_word();
    processedWaitWord = true;
  }
  if (!processedUnpressed) {
    if (isDah(pressedTime)) {
      send_dah();
    } else if (isDit(pressedTime)) {
      send_dit();
    }

    processedUnpressed = true;
  }
}

void send_dit() {
  // add dit to currentLetter increment letterIndex
  currentLetter <<= 1;
  letterIndex++;
  shouldRedraw = true;
}

void send_dah() {
  // add dah to currentLetter, increment letterIndex
  currentLetter |= 1;
  currentLetter <<= 1;
  letterIndex++;
  shouldRedraw = true;
}

void send_word() {
  // send a space
  displayString += " ";
  send_keypress((int)' ');
  shouldRedraw = true;
}

void send_char() {
  // collect dits and dahs and translate 
  Serial.print("current letter: "); Serial.println(currentLetter);
  int value = KEYMAP[letterIndex-1][currentLetter>>1];

  displayString += (char)value;
  send_keypress(value);

  // reset currentLetter and letterIndex
  currentLetter = 0;
  letterIndex = 0;
  shouldRedraw = true;
}

void send_keypress(int key) {
  if (!keyboardEnabled) {
    return;
  }
  Keyboard.write(key);
}

void print_click_status(String beginMessage) {
  Serial.println("|--------------------------------|");
  Serial.println(beginMessage);
  Serial.print("input pressed: ");
  Serial.println(inputPressed);

  Serial.print("begin press: ");
  Serial.println(startPressed);


  Serial.print("end pressed: ");
  Serial.println(endPressed);


  Serial.print("start wait: ");
  Serial.println(startWait);


  Serial.print("end wait: ");
  Serial.println(endWait);

  if (inputPressed) {
    Serial.print("Last wait time: ");
    Serial.println(waitTime / 10);
  } else {
    Serial.print("Last press time: ");
    Serial.println(pressedTime / 10);
  }
  Serial.println("|--------------------------------|");
}

bool isDit(int input) {
  return withinBounds(input, DIT_LENGTH[0], DIT_LENGTH[1]);
}

bool isChar(int input) {
  return withinBounds(input, CHAR_LENGTH[0], CHAR_LENGTH[1]);
}

bool isWord(int input) {
  return withinBounds(input, WORD_LENGTH[0], WORD_LENGTH[1]);
}

bool isDah(int input) {
  return withinBounds(input, DAH_LENGTH[0], DAH_LENGTH[1]);
}

bool withinBounds(int check, int low, int high) {
  if (check >= low && check <= high) {
    return true;
  }

  return false;
}

void draw() {
  if (!shouldRedraw) {
    return;
  }

  if (displayString.length() > 10) {
    displayString = displayString.substring(displayString.length() - 10);
  }


  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,0);
  String output = "|";

  int letterState = currentLetter;
  letterState >>= 1;

  for (int i = letterIndex; i > 0; i--) {
    output += ((letterState >> i - 1) & 1) == 0 ? "." : "-";
  }

  char preview = ' ';
  if (letterIndex >= 1) {
    preview = (char)KEYMAP[letterIndex-1][letterState];
  }

  output += "|=";
  output += preview;

  display.println(output);

  display.setCursor(0, 18);
  display.println(displayString);
  display.display();


  shouldRedraw = false;
}
