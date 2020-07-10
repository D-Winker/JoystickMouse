// Joystick Mouse
//
// Read the 4 buttons, 1 switch, and 3 potentiometers and 
//
// Left Thumb: left click and ctrl+shift+tab
// Right Thumb: right click and ctrl+tab
// Top Trigger: middle click and alt+tab
// Bottom Trigger: alternate functions while held (maybe using it as a toggle would be better? Wouldn't work well for alt-tabbing)
// Switch: no current use
// Slider: no current use
//
// TODO: Replace the web of if statements with a state machine?
// TODO: Maybe shrink deadzone? Or add some kind of filter? Make scaling nonlinear?
//
// Slider: 0 up to 1023
// Joystick
// Side to side: 0 to 1023
//  Adjust: 315 - 934 (adjust by 619)
//  Deadzone, ~20
//  Even adjusted all the way down, it can still hit 1023
//  Even adjusted all the way up, it can still hit 38
// Up and down: 0 to 1023
//  Adjust: 364 - 935 (adjust by 571)
//  Deadzone, ~20
//  Even adjusted all the way down, it can still hit 1022
//  Even adjusted all the way up, it can still hit 23
//
// Let's center the adjusters and call the deadzone +/-12
//
// Used an online mouse rate checker and could pretty consistently hit 1 kHz; a few were in the 900 Hz.
// But, mouse movement speed is directly tied to refresh rate, and a speed of 1 at 1 kHz is still pretty fast, 
// so I'm slowing things down to 100 Hz. For reference, my laptop trackpad is 80 Hz.
// With an interval of 10 ms the update rate is 72 Hz. Interestingly, if I move at a diagonal, it goes to 144 Hz!
// I guess each axis' update counts. (changed the way I send things so now it sends both at once).
// Internet says a call to millis() should take ~2 microseconds, but refresh rates aren't working as I'd expect.
//  10ms = 72 Hz (144 diagonal), 5ms = 144 Hz (292 diagonal) 0 ms = 732 Hz (diagonal or not!)
// I took out the rate-limiting if statement (and calls to millis()) and only hit 740 Hz. Not sure how I got to 1 kHz before...
// So if I take out everything, and just make it wiggle the mouse in each iteration of the loop, it works at 1 kHz.
// Turns out we only get that extra slowing when the mouse speed is 1. Mystery solved?
// Hey! - After I changed the mouse command to one command for both axes, speed 1 now occasionally hits 100 Hz!
//
// Daniel Winker, February 15, 2020

#include <Keyboard.h>
#include <Mouse.h>
#include <Bounce2.h>

int mouseSpeed = A0;  // Slider potentiometer output
int mouseHorizontal = A1;  // Horizontal potentiometer output
int mouseVertical = A2;  // Vertical potentiometer output

Bounce topTrigger = Bounce(); // Instantiate a Bounce object
Bounce bottomTrigger = Bounce(); // Instantiate a Bounce object
Bounce rightMouse = Bounce(); // Instantiate a Bounce object
Bounce sideSwitch = Bounce(); // Instantiate a Bounce object
Bounce leftMouse = Bounce(); // Instantiate a Bounce object

int vertZero = 512;  // Zero positions of each axis
int horzZero = 512;  // We can set these to constants (vs reading them) because we can physically adjust the joystick
int deadzone = 12;  // +/- deadzone on either side of zero
//TODO: Ignore the deadzone if the mouse is moving (say if one axis is more than 2x the deadzone, then ignore the deadzone on the other axis, or make it much smaller, and don't stop motion until the axis/es hit 512)
int vertValue, horzValue;  // Stores current analog output of each axis
int sensitivity = 0;
int leftClickFlag = 0;
int rightClickFlag = 0;
int ctrlClickFlag = 0;
int middleClickFlag = 0;
bool altMode = false;  // Alter the operation of the other buttons

unsigned long startTime = millis ();
unsigned long tempTime = millis ();
const unsigned long interval = 10;
int mouseX = 0;
int mouseY = 0;

void setup() {
  //Serial.begin(9600);
  pinMode(A0, INPUT);
  pinMode(A1, INPUT);
  pinMode(A2, INPUT);

  topTrigger.attach(3,INPUT_PULLUP); // Attach the debouncer to a pin with INPUT_PULLUP mode
  topTrigger.interval(25); // Use a debounce interval of 25 milliseconds
  bottomTrigger.attach(4,INPUT_PULLUP); // Attach the debouncer to a pin with INPUT_PULLUP mode
  bottomTrigger.interval(25); // Use a debounce interval of 25 milliseconds
  rightMouse.attach(5,INPUT_PULLUP); // Attach the debouncer to a pin with INPUT_PULLUP mode
  rightMouse.interval(25); // Use a debounce interval of 25 milliseconds
  sideSwitch.attach(6,INPUT_PULLUP); // Attach the debouncer to a pin with INPUT_PULLUP mode
  sideSwitch.interval(25); // Use a debounce interval of 25 milliseconds
  leftMouse.attach(7,INPUT_PULLUP); // Attach the debouncer to a pin with INPUT_PULLUP mode
  leftMouse.interval(25); // Use a debounce interval of 25 milliseconds
  delay(1000);

  sensitivity = analogRead(mouseSpeed);
}

void loop() {
  tempTime = millis();
  if (tempTime - startTime >= interval) {
    startTime = tempTime;
    mouseX = 0;
    mouseY = 0;
    sensitivity = analogRead(mouseSpeed) >> 7;  // Scale 1024 down to 8
    if (sensitivity < 1){
      sensitivity = 1;
    }
  
    vertValue = vertZero - analogRead(mouseVertical);  // read vertical offset
    horzValue = horzZero - analogRead(mouseHorizontal);  // read horizontal offset
  
    topTrigger.update(); // Update the Bounce instance
    bottomTrigger.update(); // Update the Bounce instance
    rightMouse.update(); // Update the Bounce instance
    sideSwitch.update(); // Update the Bounce instance
    leftMouse.update(); // Update the Bounce instance
  
    // Movement
    if (vertValue > deadzone || vertValue < -deadzone) {
      //Mouse.move(0, vertValue/sensitivity, 0);  // move mouse on y axis, values -127 to 127
      mouseY = vertValue >> 5;  // Scale 512 down to 16
      if (middleClickFlag && !altMode) {  // Lower the sensitivity a bit while we're scrolling
        mouseY >>= 2;
      }
    }
    if (horzValue > deadzone || horzValue < deadzone) {
      //Mouse.move(horzValue/sensitivity, 0, 0);  // move mouse on x axis, values -127 to 127
      mouseX = horzValue >> 5;
      if (middleClickFlag && !altMode) {  // Lower the sensitivity a bit while we're scrolling
        mouseX >>= 2;
      }
    }
    if (mouseX != 0 || mouseY != 0) {
      mouseY = vertValue >> 5;  // If one axis is outside of the deadzone, use both measurements
      mouseX = horzValue >> 5;  // Allows for finer movements.
      if (horzValue > 0 && horzValue < 32 || (middleClickFlag && !altMode)) {
        horzValue = 1;
      } else if (horzValue < 0 && horzValue > -32 || (middleClickFlag && !altMode)) {
        horzValue = -1;
      }
      if (vertValue > 0 && vertValue < 32 || (middleClickFlag && !altMode)) {
        vertValue = 1;
      } else if (vertValue < 0 && vertValue > -32 || (middleClickFlag && !altMode)) {
        vertValue = -1;
      }
      Mouse.move(mouseX, mouseY, 0);  // move mouse on x, y axes, values -127 to 127
    }
  
    // Buttons and switch
    if (leftMouse.fell() && !leftClickFlag && !ctrlClickFlag) {  // Call code if button is pressed
      leftClickFlag = 1;
      if (!altMode) {
        Mouse.press(MOUSE_LEFT);  // click the left button down
      } else {
        Keyboard.release(KEY_LEFT_ALT);
        Keyboard.press(KEY_LEFT_CTRL);
        Keyboard.press(KEY_LEFT_SHIFT);
        Keyboard.press(KEY_TAB);
      }
    } else if (leftMouse.rose() && leftClickFlag && !ctrlClickFlag) {  // Button released
      leftClickFlag = 0;
      if (!altMode) {
        Mouse.release(MOUSE_LEFT);  // release the left button
      } else {
        Keyboard.release(KEY_TAB);
        Keyboard.release(KEY_LEFT_SHIFT);
        Keyboard.release(KEY_LEFT_CTRL);
        Keyboard.press(KEY_LEFT_ALT);
      }
    }
     
    if (rightMouse.fell() && !rightClickFlag) {  // Call code if button is pressed
      rightClickFlag = 1;
      if (!altMode) {
        Mouse.press(MOUSE_RIGHT);  // release the left button
      } else {
        Keyboard.release(KEY_LEFT_ALT);
        Keyboard.press(KEY_LEFT_CTRL);
        Keyboard.press(KEY_TAB);
      }
    } else if (rightMouse.rose() && rightClickFlag) {  // Button released
      rightClickFlag = 0;
      if (!altMode) {
        Mouse.release(MOUSE_RIGHT);  // release the left button
      } else {
        Keyboard.release(KEY_TAB);
        Keyboard.release(KEY_LEFT_CTRL);
        Keyboard.press(KEY_LEFT_ALT);
      }
    }

    if (topTrigger.fell() && !middleClickFlag) {  // Call code if button is pressed
      middleClickFlag = 1;
      if (!altMode) {
        Mouse.press(MOUSE_MIDDLE);  // click the left button down
      } else { // We'll use the middleClickFlag to switch to a ctrl+tab mode
        Keyboard.press(KEY_TAB);
      }
    } else if (topTrigger.rose() && middleClickFlag) {  // Button released
      middleClickFlag = 0;
      if (!altMode) {
        Mouse.release(MOUSE_MIDDLE);  // release the left button
      } else {
        Keyboard.release(KEY_TAB);
      }
    }

    if (bottomTrigger.fell() && !altMode) {  // Call code if button is pressed
      altMode = true;
      Keyboard.press(KEY_LEFT_ALT);
    } else if (bottomTrigger.rose() && altMode) {  // Button released
      altMode = false;
      // If [alt, ctrl]-tabbing, release relevant keys
      if (leftClickFlag) {
        Keyboard.release(KEY_LEFT_SHIFT);
      }
      if (rightClickFlag || leftClickFlag) {
        Keyboard.release(KEY_TAB);
      }
      // Release the relevant -tabbing- key (alt or ctrl)
      if (middleClickFlag) {  // Does it matter if I release things that aren't pressed? i.e. this 'if' might be unnecessary
        Keyboard.release(KEY_LEFT_CTRL);
      } else {
        Keyboard.release(KEY_LEFT_ALT);
      }
    }
     
    if (sideSwitch.fell()) {  // Call code if button is pressed
      if (!leftClickFlag) {
        leftClickFlag = 1;
        Mouse.press(MOUSE_LEFT);  // click the left button down
      }
    } else if (sideSwitch.rose()) {  // Button released
      leftClickFlag = 0;
      Mouse.release(MOUSE_LEFT);  // release the left button
    }
  }
}
