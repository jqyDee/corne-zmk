/* Symbols for the German macOS layout (ISO).
 *
 * ZMK sends US keycodes; macOS translates them with the active layout. These
 * names say which character comes out on a German Mac.
 *
 * Dead keys (wait for the next key, tap space for the bare char): ^ ` ´
 */
#pragma once

#define DE_EXCL  LS(N1)          // !
#define DE_DQT   LS(N2)          // "
#define DE_SECT  LS(N3)          // §
#define DE_DLLR  LS(N4)          // $
#define DE_PRCNT LS(N5)          // %
#define DE_AMPS  LS(N6)          // &
#define DE_FSLH  LS(N7)          // /
#define DE_LPAR  LS(N8)          // (
#define DE_RPAR  LS(N9)          // )
#define DE_EQUAL LS(N0)          // =
#define DE_QMARK LS(MINUS)       // ?
#define DE_GRAVE LS(EQUAL)       // `  (dead key)
#define DE_PLUS  RBKT            // +
#define DE_STAR  LS(RBKT)        // *
#define DE_HASH  NUHS            // #
#define DE_SQT   LS(NUHS)        // '
#define DE_MINUS FSLH            // -
#define DE_UNDER LS(FSLH)        // _
#define DE_SEMI  LS(COMMA)       // ;
#define DE_COLON LS(DOT)         // :
#define DE_LT    GRAVE           // <  (Mac ISO swaps this key with ^)
#define DE_GT    LS(GRAVE)       // >
#define DE_CARET NUBS            // ^  (dead key)
#define DE_AT    LA(L)           // @
#define DE_TILDE LA(N)           // ~
#define DE_LBKT  LA(N5)          // [
#define DE_RBKT  LA(N6)          // ]
#define DE_PIPE  LA(N7)          // |
#define DE_LBRC  LA(N8)          // {
#define DE_RBRC  LA(N9)          // }
#define DE_BSLH  LS(LA(N7))      // backslash
#define DE_EURO  LA(E)           // €
