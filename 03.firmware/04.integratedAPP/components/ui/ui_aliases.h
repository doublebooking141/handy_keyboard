/**
 * @file ui_aliases.h
 * @brief Human-readable aliases for SquareLine Studio generated UI elements
 *
 * This file provides meaningful names for UI elements without modifying
 * the auto-generated SquareLine files. Use these aliases in new code
 * for better readability.
 */

#ifndef UI_ALIASES_H
#define UI_ALIASES_H

// ============================================================================
// Screen Aliases
// ============================================================================

#define ui_Screen_Menu              ui_MenuScreen
#define ui_Screen_JPKeyboard        ui_JPKeyboardScreen
#define ui_Screen_AtoZKeyboard      ui_AtoZKeyboardScreen
#define ui_Screen_Cursor            ui_CursorScreen
#define ui_Screen_AnalogClock       ui_AnalogClockWithBackgroud
#define ui_Screen_DateAndTime       ui_DateAndTime
#define ui_Screen_Settings          ui_SettingScreen

// ============================================================================
// Menu Screen Elements
// ============================================================================

#define ui_Menu_KeyboardsButton     ui_KeyBoards
#define ui_Menu_ClockButton         ui_Clock
#define ui_Menu_PowerOnPCButton     ui_PowerOnPC
#define ui_Menu_LightOnButton       ui_LightOn
#define ui_Menu_SettingsButton      ui_Setthing
#define ui_Menu_Header              ui_HeaderPanel4
#define ui_Menu_DateLabel           ui_DateHolderLabel
#define ui_Menu_TimeLabel           ui_TimeHolder
#define ui_Menu_BatteryBar          ui_BatteryBar

// ============================================================================
// JPKeyboard Screen Elements
// ============================================================================

#define ui_JPKeyboard_Header        ui_HeaderPanel
#define ui_JPKeyboard_BackPanel     ui_Panel2       // Container for back button
#define ui_JPKeyboard_BackImage     ui_Image2       // Back button image (絵) -> Menu
#define ui_JPKeyboard_AtoZButton    ui_Panel17      // あ/a button -> AtoZ
#define ui_JPKeyboard_CursorButton  ui_Panel12      // <+> button -> Cursor
#define ui_JPKeyboard_CursorLabel   ui_Label7       // Label showing "<+>"

// Kana key panels
#define ui_JPKeyboard_Key_A         ui_Panel3       // あ (reference style)
#define ui_JPKeyboard_Key_Ka        ui_Panel4       // か
#define ui_JPKeyboard_Key_Sa        ui_Panel5       // さ
#define ui_JPKeyboard_Key_Ta        ui_Panel8       // た
#define ui_JPKeyboard_Key_Na        ui_Panel9       // な
#define ui_JPKeyboard_Key_Ha        ui_Panel10      // は
#define ui_JPKeyboard_Key_Ma        ui_Panel13      // ま
#define ui_JPKeyboard_Key_Ya        ui_Panel14      // や
#define ui_JPKeyboard_Key_Ra        ui_Panel15      // ら
#define ui_JPKeyboard_Key_Wa        ui_Panel19      // わ

// ============================================================================
// AtoZ Keyboard Screen Elements
// ============================================================================

#define ui_AtoZ_Header              ui_HeaderPanel1
#define ui_AtoZ_Keyboard            ui_OtherKeyboard  // LVGL keyboard widget

// ============================================================================
// Cursor Screen Elements
// ============================================================================

#define ui_Cursor_Header            ui_HeaderPanel2
#define ui_Cursor_BackPanel         ui_Panel1       // Container for back button
#define ui_Cursor_BackImage         ui_Image6       // Back button image (絵) -> Menu
#define ui_Cursor_AtoZButton        ui_Panel31      // [a] button -> AtoZ
#define ui_Cursor_AtoZLabel         ui_Label22      // Label showing "[a]"
#define ui_Cursor_JPButton          ui_Panel36      // あ button -> JPKeyboard
#define ui_Cursor_JPLabel           ui_Label27      // Label showing "あ"
#define ui_Cursor_EscKey            ui_Panel22      // ESC key (reference style)
#define ui_Cursor_EnterKey          ui_Panel24      // Enter key

// ============================================================================
// Analog Clock Screen Elements
// ============================================================================

#define ui_Clock_BackImage          ui_Image11      // Back button -> Menu
#define ui_Clock_Container          ui_AnalogClockContainer

// ============================================================================
// DateAndTime Screen Elements
// ============================================================================

#define ui_DateTime_BackLabel       ui_Back         // Back label -> Menu

// ============================================================================
// Settings Screen Elements
// ============================================================================

#define ui_Settings_Header          ui_HeaderPanel3
#define ui_Settings_Content         ui_Panel43

#endif // UI_ALIASES_H
