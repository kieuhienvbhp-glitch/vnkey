// SPDX-FileCopyrightText: 2026 Nguyen Hoang Ky <nhktmdzhg@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// Hand built compositions covering the helpers ported from bamboo-core/bamboo_utils.go: validation,
// tone and mark targets, the transformation generators, the last syllable split and the two hand
// written replacements for the Go regular expressions. Every expectation is derived from the Go code
// with the data tables generated from it; the "uo" + "w" and the "uwow" states are the ones of
// test/golden/engine_core.tsv (curated-10 and curated-48).
#include "composition.h"
#include "engine_utils.h"
#include "flattener.h"
#include "input_method.h"
#include "rule.h"
#include "transformation.h"
#include "types.h"

#include <cstdio>
#include <memory>
#include <string>
#include <variant>
#include <string_view>
#include <vector>

namespace {

    using vnkey::Composition;
    using vnkey::EffectType;
    using vnkey::InputMethod;
    using vnkey::Mark;
    using vnkey::Mode;
    using vnkey::Rule;
    using vnkey::Tone;
    using vnkey::Transformation;
    using vnkey::TransformationPtr;

    int  failures = 0;

    void check(const char* name, bool condition) {
        if (!condition) {
            std::printf("FAIL %s\n", name);
            ++failures;
        }
    }

    void checkCanvas(const char* name, const Composition& composition, vnkey::ModeSet mode, std::u32string_view expected) {
        const std::u32string actual = vnkey::flatten(composition, mode);
        if (actual != expected) {
            std::printf("FAIL %s: got %zu code points, expected %zu\n", name, actual.size(), expected.size());
            ++failures;
        }
    }

    TransformationPtr makeEffect(const TransformationPtr& target, EffectType type, std::variant<Mark, Tone> effect) {
        auto trans             = std::make_shared<Transformation>();
        trans->target          = target;
        trans->rule.effectType = type;
        trans->rule.effect     = effect;
        return trans;
    }

    TransformationPtr makeRuleEffect(const TransformationPtr& target, const Rule& rule) {
        auto trans    = std::make_shared<Transformation>();
        trans->target = target;
        trans->rule   = rule;
        return trans;
    }

    TransformationPtr makeAppending(char32_t key, char32_t effectOn, bool isUpperCase) {
        auto trans             = std::make_shared<Transformation>();
        trans->isUpperCase     = isUpperCase;
        trans->rule.key        = key;
        trans->rule.effectType = EffectType::Appending;
        trans->rule.effectOn   = effectOn;
        trans->rule.result     = effectOn;
        return trans;
    }

    std::vector<Rule> rulesForKey(const InputMethod& method, char32_t key) {
        std::vector<Rule> rules;
        rules.reserve(method.rules.size());
        for (const auto& rule : method.rules) {
            if (rule.key == key) {
                rules.push_back(rule);
            }
        }
        return rules;
    }

    Composition appendAll(const Composition& composition, const Composition& extra) {
        Composition result = composition;
        result.insert(result.end(), extra.begin(), extra.end());
        return result;
    }

    Rule toneRule(char32_t key, Tone tone) {
        Rule rule;
        rule.key        = key;
        rule.effectType = EffectType::ToneTransformation;
        rule.setTone(tone);
        return rule;
    }

    Rule markRule(char32_t key, char32_t effectOn, char32_t result, Mark mark) {
        Rule rule;
        rule.key        = key;
        rule.effectType = EffectType::MarkTransformation;
        rule.effectOn   = effectOn;
        rule.result     = result;
        rule.setMark(mark);
        return rule;
    }

    constexpr vnkey::ModeSet     kVietnamese = vnkey::toMask(Mode::VietnameseMode);
    constexpr vnkey::EngineFlags kStdFlags   = vnkey::kEstdFlags;

} // namespace

int run_engine_utils() {
    const InputMethod       telex  = vnkey::cachedInputMethod(vnkey::InputMethodId::Telex);
    const std::vector<Rule> wRules = rulesForKey(telex, U'w');
    const std::vector<Rule> oRules = rulesForKey(telex, U'o');

    // The Telex "w" line is "UOA_ƯƠĂ": 6 tone rules for 'u', the "already horned" 'ư' rule, then
    // 13 rules for 'o' ('o', then 'ô', then 'ơ' tone variants) and 13 for 'a'. The "o" line is
    // "O_Ô": 13 hat rules for the 'o' family ['o', 'ô', 'ơ'] (see the generated mark families).
    check("telex w rule count", wRules.size() == 33U);
    check("telex o rule count", oRules.size() == 13U);
    check("telex w horn on u", !wRules.empty() && wRules[0].mark() == Mark::Horn && wRules[0].effectOn == U'u' && wRules[0].result == U'ư');
    check("telex w horn on o", wRules.size() > 7U && wRules[7].mark() == Mark::Horn && wRules[7].effectOn == U'o' && wRules[7].result == U'ơ');
    check("telex o hat on o", !oRules.empty() && oRules[0].mark() == Mark::Hat && oRules[0].effectOn == U'o' && oRules[0].result == U'ô');
    check("telex o hat on ơ", oRules.size() > 7U && oRules[7].mark() == Mark::Hat && oRules[7].effectOn == U'ơ' && oRules[7].result == U'ô');

    // isValid: a plain syllable and the grave tone on a "c" coda, which only accepts an acute or dot.
    {
        const Composition ba = {vnkey::makeAppendingTrans(U'b', false), vnkey::makeAppendingTrans(U'a', false)};
        check("isValid ba", vnkey::isValid(ba, false));
        check("isValid ba complete", vnkey::isValid(ba, true));

        const auto        a  = vnkey::makeAppendingTrans(U'a', false);
        const Composition ac = {a, makeEffect(a, EffectType::ToneTransformation, Tone::Grave), vnkey::makeAppendingTrans(U'c', false)};
        check("isValid ac with grave", !vnkey::isValid(ac, false));
        check("isValid empty", vnkey::isValid({}, false));
    }

    // findToneTarget: one vowel, then the modern ("std") style moving the tone of "oa" to the first
    // vowel, and the last consonant forcing the second one.
    {
        const auto        a  = vnkey::makeAppendingTrans(U'a', false);
        const auto        o  = vnkey::makeAppendingTrans(U'o', false);
        const auto        n  = vnkey::makeAppendingTrans(U'n', false);
        const Composition ba = {vnkey::makeAppendingTrans(U'b', false), a};
        check("tone target empty", vnkey::findToneTarget({}, false) == nullptr);
        check("tone target single vowel", vnkey::isSame(vnkey::findToneTarget(ba, false), a));

        const Composition oa = {o, a};
        check("tone target oa old style", vnkey::isSame(vnkey::findToneTarget(oa, false), a));
        check("tone target oa std style", vnkey::isSame(vnkey::findToneTarget(oa, true), o));

        const Composition oan = {o, a, n};
        check("tone target oan old style", vnkey::isSame(vnkey::findToneTarget(oan, false), a));
    }

    // hasValidTone: acute and dot are always free, the others depend on the last consonant.
    {
        const auto        a      = vnkey::makeAppendingTrans(U'a', false);
        const auto        c      = vnkey::makeAppendingTrans(U'c', false);
        const auto        h      = vnkey::makeAppendingTrans(U'h', false);
        const auto        n      = vnkey::makeAppendingTrans(U'n', false);
        const auto        grave  = makeEffect(a, EffectType::ToneTransformation, Tone::Grave);
        const auto        dot    = makeEffect(a, EffectType::ToneTransformation, Tone::Dot);
        const Composition aGrave = {a, grave};
        check("hasValidTone acute", vnkey::hasValidTone(aGrave, Tone::Acute));
        check("hasValidTone dot with c", vnkey::hasValidTone({a, dot, c}, Tone::Dot));
        check("hasValidTone grave open", vnkey::hasValidTone(aGrave, Tone::Grave));
        check("hasValidTone grave c", !vnkey::hasValidTone(appendAll(aGrave, {c}), Tone::Grave));
        check("hasValidTone grave ch", !vnkey::hasValidTone(appendAll(aGrave, {c, h}), Tone::Grave));
        check("hasValidTone grave n", vnkey::hasValidTone(appendAll(aGrave, {n}), Tone::Grave));
    }

    // findTarget: the first tone rule wins, mark rules are found afterwards, and a key without any
    // applicable rule returns a null target with a default constructed rule (Go's nil, Rule{}).
    {
        const auto              a         = vnkey::makeAppendingTrans(U'a', false);
        const Rule              tRule     = toneRule(U'f', Tone::Grave);
        const vnkey::TargetRule toneFound = vnkey::findTarget({a}, {tRule}, kStdFlags);
        check("findTarget tone target", vnkey::isSame(toneFound.target, a));
        check("findTarget tone rule", toneFound.rule.effectType == EffectType::ToneTransformation && toneFound.rule.tone() == Tone::Grave && toneFound.rule.key == U'f');

        const Rule              mRule     = markRule(U'w', U'a', U'ă', Mark::Breve);
        const vnkey::TargetRule markFound = vnkey::findTarget({a}, {mRule}, kStdFlags);
        check("findTarget mark target", vnkey::isSame(markFound.target, a));
        check("findTarget mark rule", markFound.rule.effectType == EffectType::MarkTransformation && markFound.rule.mark() == Mark::Breve && markFound.rule.effectOn == U'a');

        const vnkey::TargetRule noneFound = vnkey::findTarget({vnkey::makeAppendingTrans(U'b', false)}, {tRule}, vnkey::EngineFlags{});
        check("findTarget nothing target", noneFound.target == nullptr);
        check("findTarget nothing rule", noneFound.rule.key == 0 && noneFound.rule.mark() == Mark::None && noneFound.rule.effectType == EffectType::Appending);
    }

    // generateTransformations: "uo" + "w" (golden curated-10), double typing an effect key, and
    // "ươ" + "o", which goes through the ưo/ươ shortcut (golden curated-48 is the "uwow" state).
    {
        const auto        u         = vnkey::makeAppendingTrans(U'u', false);
        const auto        o         = vnkey::makeAppendingTrans(U'o', false);
        const Composition uo        = {u, o};
        const Composition uoApplied = vnkey::generateTransformations(uo, wRules, kStdFlags, U'w', false);
        check("uo+w size", uoApplied.size() == 1U);
        check("uo+w horn on o", uoApplied.size() == 1U && uoApplied[0]->rule.mark() == Mark::Horn && uoApplied[0]->rule.effectOn == U'o' && vnkey::isSame(uoApplied[0]->target, o));
        checkCanvas("uo+w", appendAll(uo, uoApplied), kVietnamese, U"uơ");

        const auto        doubleKey = makeAppending(U'w', U'ư', false);
        const Composition doubled   = vnkey::generateTransformations({doubleKey}, wRules, kStdFlags, U'w', false);
        check("double key size", doubled.size() == 1U);
        check("double key raw",
              doubled.size() == 1U && doubled[0]->rule.effectType == EffectType::MarkTransformation && doubled[0]->rule.mark() == Mark::Raw &&
                  vnkey::isSame(doubled[0]->target, doubleKey));
        checkCanvas("double key", appendAll({doubleKey}, doubled), kVietnamese, U"w");

        // The engine reaches this state by typing "uwow": a horn node on "u", the "o" that was
        // appended next, and a horn node on "o".
        const Composition uoHorned = {u, makeRuleEffect(u, wRules[0]), o, makeRuleEffect(o, wRules[7])};
        checkCanvas("uwow state", uoHorned, kVietnamese, U"ươ");
        check("uwow enters the ưo/ươ shortcut", vnkey::matchesUhO(vnkey::flatten(uoHorned, kVietnamese | vnkey::toMask(Mode::ToneLess) | vnkey::toMask(Mode::LowerCase))));
        const Composition uoHatted = vnkey::generateTransformations(uoHorned, oRules, kStdFlags, U'o', false);
        check("ươ+o size", uoHatted.size() == 2U);
        check("ươ+o clears the first vowel", uoHatted.size() == 2U && uoHatted[0]->rule.mark() == Mark::None && vnkey::isSame(uoHatted[0]->target, u));
        check("ươ+o hats the second vowel", uoHatted.size() == 2U && uoHatted[1]->rule.mark() == Mark::Hat && vnkey::isSame(uoHatted[1]->target, o));
        checkCanvas("ươ+o", appendAll(uoHorned, uoHatted), kVietnamese, U"uô");
    }

    // generateUndoTransformations: an effect key that cannot be applied undoes its effect.
    {
        const auto        a         = vnkey::makeAppendingTrans(U'a', false);
        const auto        grave     = makeEffect(a, EffectType::ToneTransformation, Tone::Grave);
        const Composition aGrave    = {a, grave};
        const Rule        graveRule = toneRule(U'f', Tone::Grave);
        const Composition toneUndo  = vnkey::generateUndoTransformations(aGrave, {graveRule}, kStdFlags);
        check("undo tone size", toneUndo.size() == 1U);
        check("undo tone node",
              toneUndo.size() == 1U && toneUndo[0]->rule.effectType == EffectType::ToneTransformation && toneUndo[0]->rule.tone() == Tone::None &&
                  vnkey::isSame(toneUndo[0]->target, a));
        checkCanvas("undo tone applied", appendAll(aGrave, toneUndo), kVietnamese, U"a");

        const auto        hat      = makeRuleEffect(a, markRule(U'a', U'a', U'â', Mark::Hat));
        const Composition aHat     = {a, hat};
        const Rule        hatRule  = markRule(U'a', U'â', U'a', Mark::Hat);
        const Composition markUndo = vnkey::generateUndoTransformations(aHat, {hatRule}, kStdFlags);
        check("undo mark size", markUndo.size() == 1U);
        check("undo mark node",
              markUndo.size() == 1U && markUndo[0]->rule.effectType == EffectType::MarkTransformation && markUndo[0]->rule.mark() == Mark::None &&
                  vnkey::isSame(markUndo[0]->target, a));
        checkCanvas("undo mark applied", appendAll(aHat, markUndo), kVietnamese, U"a");
        check("undo mark nothing", vnkey::generateUndoTransformations({a}, {hatRule}, kStdFlags).empty());
    }

    // generateFallbackTransformations: the appending node, its case, and the virtual appended rules.
    {
        const Composition plain = vnkey::generateFallbackTransformations({}, U'z', false);
        check("fallback plain size", plain.size() == 1U);
        checkCanvas("fallback plain", plain, kVietnamese, U"z");

        Rule appending;
        appending.key        = U'w';
        appending.effectType = EffectType::Appending;
        appending.effectOn   = U'ư';
        appending.result     = U'ư';
        Rule appended;
        appended.key        = U'w';
        appended.effectType = EffectType::Appending;
        appended.effectOn   = U'ơ';
        appended.result     = U'ơ';
        appending.appendedRules.push_back(appended);
        const Composition withAppended = vnkey::generateFallbackTransformations({appending}, U'w', false);
        check("fallback appended size", withAppended.size() == 2U);
        check("fallback appended virtual key", withAppended.size() == 2U && withAppended[1]->rule.key == 0 && withAppended[1]->rule.effectOn == U'ơ');
        checkCanvas("fallback appended", withAppended, kVietnamese, U"ư");

        Rule upper;
        upper.key        = U'w';
        upper.effectType = EffectType::Appending;
        upper.effectOn   = U'Ư';
        upper.result     = U'Ư';
        checkCanvas("fallback upper", vnkey::generateFallbackTransformations({upper}, U'w', false), kVietnamese, U"Ư");
    }

    // getRightMostVowels and refreshLastToneTarget: "òa" moves its tone to the second vowel.
    {
        const auto        u  = vnkey::makeAppendingTrans(U'u', false);
        const auto        o  = vnkey::makeAppendingTrans(U'o', false);
        const auto        a  = vnkey::makeAppendingTrans(U'a', false);
        const Composition uo = {u, o};
        check("right most vowels", vnkey::rightMostVowels(uo).size() == 2U);
        check("right most vowels empty", vnkey::rightMostVowels({}).empty());
        check("right most vowels consonants only", vnkey::rightMostVowels({vnkey::makeAppendingTrans(U'b', false)}).empty());

        const auto        graveOnO = makeEffect(o, EffectType::ToneTransformation, Tone::Grave);
        const auto        graveOnA = makeEffect(a, EffectType::ToneTransformation, Tone::Grave);
        const Composition moved    = vnkey::refreshLastToneTarget({o, a, graveOnO}, false);
        check("refresh size", moved.size() == 2U);
        check("refresh clears the old target", moved.size() == 2U && vnkey::isSame(moved[0]->target, o) && moved[0]->rule.tone() == Tone::None);
        check("refresh moves to the new target", moved.size() == 2U && vnkey::isSame(moved[1]->target, a) && moved[1]->rule.tone() == Tone::Grave);
        checkCanvas("refresh moved", appendAll({o, a, graveOnO}, moved), kVietnamese, U"oà");
        check("refresh keeps the target", vnkey::refreshLastToneTarget({o, a, graveOnA}, false).empty());
    }

    // extractLastSyllable: the punctuation mark stays with the previous part, and an invalid tail
    // is moved there as well.
    {
        const auto             b        = vnkey::makeAppendingTrans(U'b', false);
        const auto             a        = vnkey::makeAppendingTrans(U'a', false);
        const auto             space    = vnkey::makeAppendingTrans(U' ', false);
        const Composition      twoWords = {b, a, space, vnkey::makeAppendingTrans(U'b', false), vnkey::makeAppendingTrans(U'a', false)};
        const vnkey::WordSplit split    = vnkey::extractLastSyllable(twoWords);
        check("extractLastSyllable two words previous", split.previous.size() == 3U);
        check("extractLastSyllable two words last", split.last.size() == 2U);

        const Composition      punctuation = {b, a, vnkey::makeAppendingTrans(U'.', false), vnkey::makeAppendingTrans(U'b', false), vnkey::makeAppendingTrans(U'a', false)};
        const vnkey::WordSplit punctSplit  = vnkey::extractLastSyllable(punctuation);
        check("extractLastSyllable punctuation previous", punctSplit.previous.size() == 3U);
        check("extractLastSyllable punctuation last", punctSplit.last.size() == 2U);

        const Composition      brokenTail  = {b, a, space, vnkey::makeAppendingTrans(U'b', false), vnkey::makeAppendingTrans(U'b', false)};
        const vnkey::WordSplit anchorSplit = vnkey::extractLastSyllable(brokenTail);
        check("extractLastSyllable anchor previous", anchorSplit.previous.size() == 4U);
        check("extractLastSyllable anchor last", anchorSplit.last.size() == 1U);
        check("extractLastSyllable anchor node", anchorSplit.last.size() == 1U && vnkey::isSame(anchorSplit.last[0], brokenTail[4]));
    }

    // The hand written replacements of the two Go regular expressions (`regUOhTail` and `regUhO`).
    {
        check("uơ tail ends the text", !vnkey::matchesUOhTail(U"uơ"));
        check("uơ tail with a letter", vnkey::matchesUOhTail(U"uơn"));
        check("uơ tail with a digit", !vnkey::matchesUOhTail(U"uơ1"));
        check("uơ tail inside a word", vnkey::matchesUOhTail(U"muơn"));
        check("ưo with a letter", vnkey::matchesUOhTail(U"ưong"));
        check("ưo ends the text", !vnkey::matchesUOhTail(U"ưo"));
        check("ưo before a space", !vnkey::matchesUOhTail(U"ưo "));
        check("uơ tail rejects ưa", !vnkey::matchesUOhTail(U"xưa"));
        check("uơ tail rejects empty", !vnkey::matchesUOhTail(U""));

        check("ưo pattern", vnkey::matchesUhO(U"ưo"));
        check("ươ pattern", vnkey::matchesUhO(U"ươ"));
        check("ưo pattern inside a word", vnkey::matchesUhO(U"xươn"));
        check("ưo pattern rejects uơ", !vnkey::matchesUhO(U"uơ"));
        check("ưo pattern rejects a bare ư", !vnkey::matchesUhO(U"ư"));
        check("ưo pattern rejects empty", !vnkey::matchesUhO(U""));
    }

    if (failures != 0) {
        std::printf("%d failure(s)\n", failures);
        return 1;
    }
    std::printf("vnkey engine utils: all checks passed\n");
    return 0;
}
