#include <string.h>
#include "app/messenger_t9.h"

#define MSG_T9_COMMIT_TICKS 80U

// Таблица подстановок для МАЛЕНЬКИХ русских букв
static const char* const t9_keys_low[] = {
    " ",
    ".,?!1",
    "\xA0\xA1\xA2\xA3\x12", // абвг2
    "\xA4\xA5\xA6\xA7\x13", // дежз3
    "\xA8\xA9\xAA\xAB\x14", // ийкл4
    "\xAC\xAD\xAE\xAF\x15", // мноп5
    "\xE0\xE1\xE2\xE3\x16", // рсту6
    "\xE4\xE5\xE6\xE7\x17", // фхцч7
    "\xE8\xE9\xEA\xEB\x18", // шщъы8
    "\xEC\xED\xEE\xEF\x19" // ьэюя9
};

// Таблица подстановок для ЗАГЛАВНЫХ (БОЛЬШИХ) русских букв
static const char* const t9_keys_up[] = {
    " ",
    ".,?!1",
    "\x80\x81\x82\x83\x12", // АБВГ2
    "\x84\x85\x86\x87\x13", // ДЕЖЗ3
    "\x88\x89\x8A\x8B\x14", // ИЙКЛ4
    "\x8C\x8D\x8E\x8F\x15", // МНОП5
    "\x90\x91\x92\x93\x16", // РСТУ6
    "\x94\x95\x96\x97\x17", // ФХЦЧ7
    "\x98\x99\x9A\x9B\x18", // ШЩЪЫ8
    "\x9C\x9D\x9E\x9F\x19" // ЬЭЮЯ9
};

// Функция привязки букв к кнопкам клавиатуры
static const char* map_chars(KEY_Code_t key)
{
    switch (key) {
        case KEY_0: return " ";
        case KEY_1: return ".,?!1";
        case KEY_2: return "\x80\x81\x82\x83\x12"; // АБВГ2
        case KEY_3: return "\x84\x85\x86\x87\x13"; // ДЕЖЗ3
        case KEY_4: return "\x88\x89\x8A\x8B\x14"; // ИЙКЛ4
        case KEY_5: return "\x8C\x8D\x8E\x8F\x15"; // МНОП5
        case KEY_6: return "\x90\x91\x92\x93\x16"; // РСТУ6
        case KEY_7: return "\x94\x95\x96\x97\x17"; // ФХЦЧ7
        case KEY_8: return "\x98\x99\x9A\x9B\x18"; // ШЩЪЫ8
        case KEY_9: return "\x9C\x9D\x9E\x9F\x19"; // ЬЭЮЯ9
        default: return "";
    }
}

static char apply_case(char c, bool upper)
{
    if (!upper && c >= 'A' && c <= 'Z') c = c + ('a' - 'A');
    return c;
}

void MSG_T9_Start(MSG_T9Editor_t *ed, char *buf, uint8_t max_len)
{
    ed->buf = buf;
    ed->max_len = max_len;
    ed->strlen = strlen(buf);
    if (ed->strlen > max_len) ed->strlen = max_len;
    ed->upper = true;
    ed->mode = 0;
    ed->pending_key = KEY_INVALID;
    ed->cycle_index = 0;
    ed->pending_ticks = 0;
    ed->has_pending = false;
}

void MSG_T9_Commit(MSG_T9Editor_t *ed)
{
    if (!ed) return;

    ed->pending_key = KEY_INVALID;
    ed->cycle_index = 0;
    ed->pending_ticks = 0;
    ed->has_pending = false;
}

void MSG_T9_Tick(MSG_T9Editor_t *ed)
{
    if (!ed || !ed->has_pending) return;

    if (++ed->pending_ticks >= MSG_T9_COMMIT_TICKS) MSG_T9_Commit(ed);
}

bool MSG_T9_HandleKey(MSG_T9Editor_t *ed, KEY_Code_t key)
{
    if (!ed || !ed->buf) return false;

    if (key == KEY_STAR) {
        MSG_T9_Commit(ed);
        ed->mode = (uint8_t)((ed->mode + 1U) % 3U);
        ed->upper = (ed->mode == 0U);
        return true;
    }

    if (key == KEY_F || key == KEY_EXIT) {
        if (ed->strlen > 0) ed->buf[--ed->strlen] = 0;
        MSG_T9_Commit(ed);
        return true;
    }

    const char *chars = map_chars(key);
    uint8_t n = (uint8_t)strlen(chars);
    if (n == 0) return false;

    /* Numeric mode: STAR cycles B -> b -> 2. In mode 2, each keypad
     * press inserts the digit immediately; no multi-tap pending state. */
    if (ed->mode == 2U) {
        MSG_T9_Commit(ed);
        if (ed->strlen >= ed->max_len) return true;
        if (key >= KEY_0 && key <= KEY_9) {
            ed->buf[ed->strlen++] = (char)('0' + (uint8_t)key);
            ed->buf[ed->strlen] = 0;
            return true;
        }
    }

    if (ed->has_pending && ed->pending_key == key && ed->strlen > 0) {
        ed->cycle_index = (uint8_t)((ed->cycle_index + 1U) % n);
        ed->buf[ed->strlen - 1U] = apply_case(chars[ed->cycle_index], ed->upper);
        ed->pending_ticks = 0;
        return true;
    }

    MSG_T9_Commit(ed);

    if (ed->strlen >= ed->max_len) return true;

    ed->cycle_index = 0;
    ed->pending_key = key;
    ed->has_pending = true;
    ed->pending_ticks = 0;
    ed->buf[ed->strlen++] = apply_case(chars[ed->cycle_index], ed->upper);
    ed->buf[ed->strlen] = 0;
    return true;
}

bool MSG_T9_HandleLongKey(MSG_T9Editor_t *ed, KEY_Code_t key)
{
    if (!ed || !ed->buf) return false;
    if (key < KEY_0 || key > KEY_9) return false;

    /* В режимах 0/1 долгое нажатие цифровой клавиши вставляет цифру напрямую. */
    MSG_T9_Commit(ed);
    if (ed->strlen >= ed->max_len) return true;
    ed->buf[ed->strlen++] = (char)('0' + (uint8_t)key);
    ed->buf[ed->strlen] = 0;
    return true;
}
