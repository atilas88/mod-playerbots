/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#include "RaidAnnounceParser.h"

#include <algorithm>
#include <iterator>

#include "SharedDefines.h"

namespace
{
constexpr uint32 MIN_ANNOUNCED_GEAR_SCORE = 2000;
constexpr uint32 MAX_ANNOUNCED_GEAR_SCORE = 7000;
constexpr size_t MAX_RECRUIT_SLOTS = 5;
constexpr size_t LINK_COLOR_PREFIX_LENGTH = 10;  // "|cffa335ee"
constexpr std::string_view CUT_TOKEN = ",";

enum class TokenKind : uint8
{
    Other,
    Intent,
    Role,
    Class,
    Spec,
    Instance,
    Size,
    Heroic,
    Cut
};

struct Token
{
    TokenKind kind = TokenKind::Other;
    RecruitRole role = RecruitRole::Dps;
    uint8 classId = 0;
    std::string_view instance;
    uint8 size = 0;
    bool heroic = false;
};

struct RoleWord
{
    std::string_view word;
    RecruitRole role;
};

struct ClassWord
{
    std::string_view word;
    uint8 classId;
};

struct SpecWord
{
    std::string_view word;
    uint8 classId;
    RecruitRole role;
};

struct InstanceWord
{
    std::string_view word;
    std::string_view key;
    bool heroic;
};

constexpr std::string_view INTENT_WORDS[] = {"need", "needs", "lf", "lfm"};

// "looking for <these>" is a player offering themselves, not recruiting
constexpr std::string_view LOOKING_FOR_SELF_WORDS[] = {"group", "guild", "raid", "party", "team"};

constexpr std::string_view SKIPPED_ARTICLES[] = {"a", "an", "one", "more", "some", "any"};

constexpr std::string_view CUT_WORDS[] = {"and", "or"};

constexpr std::string_view MONEY_WORDS[] = {"g", "gold", "golds"};

constexpr std::string_view GEAR_SCORE_WORDS[] = {"gs", "gearscore"};

constexpr std::string_view HEROIC_WORDS[] = {"hc", "heroic"};

constexpr RoleWord ROLE_WORDS[] = {
    {"tank", RecruitRole::Tank},    {"tanks", RecruitRole::Tank}, {"mt", RecruitRole::Tank},
    {"ot", RecruitRole::Tank},      {"heal", RecruitRole::Heal},  {"heals", RecruitRole::Heal},
    {"healer", RecruitRole::Heal},  {"healers", RecruitRole::Heal}, {"dps", RecruitRole::Dps},
    {"rdps", RecruitRole::Dps},     {"mdps", RecruitRole::Dps},   {"dd", RecruitRole::Dps},
    {"dds", RecruitRole::Dps},
};

constexpr ClassWord CLASS_WORDS[] = {
    {"warrior", CLASS_WARRIOR},     {"warriors", CLASS_WARRIOR},         {"warr", CLASS_WARRIOR},
    {"war", CLASS_WARRIOR},         {"paladin", CLASS_PALADIN},          {"paladins", CLASS_PALADIN},
    {"pala", CLASS_PALADIN},        {"pally", CLASS_PALADIN},            {"pal", CLASS_PALADIN},
    {"hunter", CLASS_HUNTER},       {"hunters", CLASS_HUNTER},           {"hunt", CLASS_HUNTER},
    {"rogue", CLASS_ROGUE},         {"rogues", CLASS_ROGUE},             {"priest", CLASS_PRIEST},
    {"priests", CLASS_PRIEST},      {"dk", CLASS_DEATH_KNIGHT},          {"dks", CLASS_DEATH_KNIGHT},
    {"deathknight", CLASS_DEATH_KNIGHT}, {"shaman", CLASS_SHAMAN},       {"shamans", CLASS_SHAMAN},
    {"sham", CLASS_SHAMAN},         {"shammy", CLASS_SHAMAN},            {"mage", CLASS_MAGE},
    {"mages", CLASS_MAGE},          {"warlock", CLASS_WARLOCK},          {"warlocks", CLASS_WARLOCK},
    {"lock", CLASS_WARLOCK},        {"locks", CLASS_WARLOCK},            {"druid", CLASS_DRUID},
    {"druids", CLASS_DRUID},        {"dudu", CLASS_DRUID},
};

// Only spec nicknames that name one class and one role; "resto", "holy" and "prot" fit several classes
constexpr SpecWord SPEC_WORDS[] = {
    {"boomkin", CLASS_DRUID, RecruitRole::Dps},   {"moonkin", CLASS_DRUID, RecruitRole::Dps},
    {"balance", CLASS_DRUID, RecruitRole::Dps},   {"rdruid", CLASS_DRUID, RecruitRole::Heal},
    {"spriest", CLASS_PRIEST, RecruitRole::Dps},  {"shadow", CLASS_PRIEST, RecruitRole::Dps},
    {"disc", CLASS_PRIEST, RecruitRole::Heal},    {"hpal", CLASS_PALADIN, RecruitRole::Heal},
    {"hpala", CLASS_PALADIN, RecruitRole::Heal},  {"hpally", CLASS_PALADIN, RecruitRole::Heal},
    {"ret", CLASS_PALADIN, RecruitRole::Dps},     {"retri", CLASS_PALADIN, RecruitRole::Dps},
    {"rsham", CLASS_SHAMAN, RecruitRole::Heal},   {"rshaman", CLASS_SHAMAN, RecruitRole::Heal},
    {"enh", CLASS_SHAMAN, RecruitRole::Dps},      {"ele", CLASS_SHAMAN, RecruitRole::Dps},
    {"bdk", CLASS_DEATH_KNIGHT, RecruitRole::Tank},
};

constexpr InstanceWord INSTANCE_WORDS[] = {
    {"icc", "icc", false},      {"icecrown", "icc", false},   {"toc", "toc", false},
    {"totc", "toc", false},     {"togc", "toc", true},        {"voa", "voa", false},
    {"archavon", "voa", false}, {"naxx", "naxx", false},      {"nax", "naxx", false},
    {"naxxramas", "naxx", false}, {"uld", "uld", false},      {"ulda", "uld", false},
    {"ulduar", "uld", false},   {"rs", "rs", false},          {"ruby", "rs", false},
    {"halion", "rs", false},    {"os", "os", false},          {"sarth", "os", false},
    {"sartharion", "os", false}, {"obsidian", "os", false},   {"eoe", "eoe", false},
    {"maly", "eoe", false},     {"malygos", "eoe", false},    {"ony", "ony", false},
    {"onyxia", "ony", false},
};

bool IsDigit(char c) { return c >= '0' && c <= '9'; }

bool IsLetter(char c) { return c >= 'a' && c <= 'z'; }

bool IsCutChar(char c) { return c == ',' || c == '/' || c == '+' || c == '&' || c == ';'; }

template <size_t N>
bool Contains(std::string_view const (&words)[N], std::string_view word)
{
    return std::find(std::begin(words), std::end(words), word) != std::end(words);
}

// Client links carry item, quest and achievement names that must not be read as keywords
std::string StripLinks(std::string_view message)
{
    std::string text(message);
    size_t start = 0;
    while ((start = text.find("|H", start)) != std::string::npos)
    {
        size_t eraseFrom = start;
        if (start >= LINK_COLOR_PREFIX_LENGTH && text.compare(start - LINK_COLOR_PREFIX_LENGTH, 2, "|c") == 0)
            eraseFrom = start - LINK_COLOR_PREFIX_LENGTH;

        size_t const end = text.find("|h|r", start);
        if (end == std::string::npos)
        {
            text.erase(eraseFrom);
            break;
        }

        text.replace(eraseFrom, end + 4 - eraseFrom, " ");
        start = eraseFrom;
    }

    return text;
}

// Lowercases ASCII; any other byte becomes a space so it can only separate words
std::string Normalize(std::string text)
{
    for (char& c : text)
    {
        if (c >= 'A' && c <= 'Z')
            c = static_cast<char>(c - 'A' + 'a');
        else if (static_cast<uint8>(c) >= 0x80)
            c = ' ';
    }

    return text;
}

std::string_view WordAt(std::string const& text, size_t pos)
{
    size_t end = pos;
    while (end < text.size() && IsLetter(text[end]))
        ++end;

    return std::string_view(text).substr(pos, end - pos);
}

size_t SkipSpaces(std::string const& text, size_t pos)
{
    while (pos < text.size() && text[pos] == ' ')
        ++pos;

    return pos;
}

// True when the letters right before pos spell "gs" or "gearscore", with or without punctuation in between
bool IsPrecededByGearScoreWord(std::string const& text, size_t pos)
{
    size_t end = pos;
    while (end > 0 && !IsLetter(text[end - 1]) && !IsDigit(text[end - 1]))
        --end;

    size_t begin = end;
    while (begin > 0 && IsLetter(text[begin - 1]))
        --begin;

    return Contains(GEAR_SCORE_WORDS, std::string_view(text).substr(begin, end - begin));
}

// Reads "5.5k", "5,8k", "5k", "5800 gs", "gs 5800" and "5800+" and blanks them so they leave no stray tokens.
// Runs before tokenizing, which would split "5.5k+" apart. Returns the highest value found, 0 if none.
uint32 ExtractGearScore(std::string& text)
{
    uint32 best = 0;
    size_t i = 0;
    while (i < text.size())
    {
        if (!IsDigit(text[i]))
        {
            ++i;
            continue;
        }

        size_t const start = i;
        while (i < text.size() && IsDigit(text[i]))
            ++i;

        size_t const digits = i - start;
        bool const afterGearScoreWord = IsPrecededByGearScoreWord(text, start);
        bool const atBoundary =
            start == 0 || (!IsLetter(text[start - 1]) && text[start - 1] != '.' && text[start - 1] != ',');
        if (!atBoundary && !afterGearScoreWord)
            continue;

        uint32 value = 0;
        size_t end = i;
        bool thousands = false;
        if (digits == 1)
        {
            value = static_cast<uint32>(text[start] - '0') * 1000;
            if (end + 1 < text.size() && (text[end] == '.' || text[end] == ',') && IsDigit(text[end + 1]))
            {
                value += static_cast<uint32>(text[end + 1] - '0') * 100;
                end += 2;
                if (end < text.size() && IsDigit(text[end]))
                {
                    value += static_cast<uint32>(text[end] - '0') * 10;
                    ++end;
                }
            }

            thousands = end < text.size() && text[end] == 'k' && (end + 1 == text.size() || !IsLetter(text[end + 1]));
            if (thousands)
                ++end;
        }
        else if (digits == 4)
        {
            for (size_t j = start; j < i; ++j)
                value = value * 10 + static_cast<uint32>(text[j] - '0');
        }

        if (!thousands && digits != 4)
            continue;

        size_t next = SkipSpaces(text, end);
        bool const plus = next < text.size() && text[next] == '+';
        if (plus)
            next = SkipSpaces(text, next + 1);

        std::string_view const following = next < text.size() ? WordAt(text, next) : std::string_view();
        if (thousands && Contains(MONEY_WORDS, following))
            continue;

        // a bare four-digit number is not a GearScore: "2500 dps"
        if (!thousands && !plus && !afterGearScoreWord && !Contains(GEAR_SCORE_WORDS, following))
            continue;

        i = std::max(i, end);
        if (value < MIN_ANNOUNCED_GEAR_SCORE || value > MAX_ANNOUNCED_GEAR_SCORE)
            continue;

        best = std::max(best, value);
        size_t const blankEnd = plus ? text.find('+', end) + 1 : end;
        std::fill(text.begin() + start, text.begin() + blankEnd, ' ');
        i = blankEnd;
    }

    return best;
}

// "lf1m", "lf2m": looking for N more
bool IsLookingForMore(std::string_view word)
{
    if (word.size() < 4 || word.substr(0, 2) != "lf" || word.back() != 'm')
        return false;

    std::string_view const count = word.substr(2, word.size() - 3);
    return std::all_of(count.begin(), count.end(), IsDigit);
}

// Splits into words, breaking "icc25" and "25hc" at the letter/digit boundary; list separators become cut tokens
std::vector<std::string_view> Tokenize(std::string const& text)
{
    std::vector<std::string_view> words;
    std::string_view const view(text);
    size_t i = 0;
    while (i < view.size())
    {
        char const c = view[i];
        if (IsCutChar(c))
        {
            words.push_back(CUT_TOKEN);
            ++i;
            continue;
        }

        if (!IsLetter(c) && !IsDigit(c))
        {
            ++i;
            continue;
        }

        size_t end = i;
        while (end < view.size() && (IsLetter(view[end]) || IsDigit(view[end])))
            ++end;

        std::string_view const word = view.substr(i, end - i);
        if (IsLookingForMore(word))
        {
            words.push_back("lfm");
            i = end;
            continue;
        }

        size_t pieceStart = i;
        for (size_t j = i + 1; j <= end; ++j)
        {
            if (j < end && IsDigit(view[j]) == IsDigit(view[j - 1]))
                continue;

            words.push_back(view.substr(pieceStart, j - pieceStart));
            pieceStart = j;
        }

        i = end;
    }

    return words;
}

Token Classify(std::string_view word, bool previousWasSize)
{
    Token token;
    if (word == CUT_TOKEN || Contains(CUT_WORDS, word))
    {
        token.kind = TokenKind::Cut;
        return token;
    }

    if (Contains(INTENT_WORDS, word))
    {
        token.kind = TokenKind::Intent;
        return token;
    }

    if (word == "10" || word == "25")
    {
        token.kind = TokenKind::Size;
        token.size = word == "10" ? 10 : 25;
        return token;
    }

    // a bare "h" only means heroic glued to the size: "25h"
    if (Contains(HEROIC_WORDS, word) || (word == "h" && previousWasSize))
    {
        token.kind = TokenKind::Heroic;
        return token;
    }

    for (RoleWord const& entry : ROLE_WORDS)
    {
        if (entry.word != word)
            continue;

        token.kind = TokenKind::Role;
        token.role = entry.role;
        return token;
    }

    for (ClassWord const& entry : CLASS_WORDS)
    {
        if (entry.word != word)
            continue;

        token.kind = TokenKind::Class;
        token.classId = entry.classId;
        return token;
    }

    for (SpecWord const& entry : SPEC_WORDS)
    {
        if (entry.word != word)
            continue;

        token.kind = TokenKind::Spec;
        token.classId = entry.classId;
        token.role = entry.role;
        return token;
    }

    for (InstanceWord const& entry : INSTANCE_WORDS)
    {
        if (entry.word != word)
            continue;

        token.kind = TokenKind::Instance;
        token.instance = entry.key;
        token.heroic = entry.heroic;
        return token;
    }

    return token;
}

// "looking for a tank" recruits; "looking for group" and "looking for a guild" do not
bool IsRecruitingLookingFor(std::vector<std::string_view> const& words, size_t index)
{
    if (index + 1 >= words.size() || words[index + 1] != "for")
        return false;

    size_t next = index + 2;
    while (next < words.size() && Contains(SKIPPED_ARTICLES, words[next]))
        ++next;

    return next < words.size() && !Contains(LOOKING_FOR_SELF_WORDS, words[next]);
}

void AddSlot(std::vector<RecruitSlot>& slots, RecruitSlot const& slot)
{
    if (slots.size() >= MAX_RECRUIT_SLOTS)
        return;

    if (std::find(slots.begin(), slots.end(), slot) == slots.end())
        slots.push_back(slot);
}
}  // namespace

std::optional<RaidAnnounce> ParseRaidAnnounce(std::string_view message)
{
    std::string text = Normalize(StripLinks(message));

    RaidAnnounce announce;
    announce.minGearScore = ExtractGearScore(text);

    std::vector<std::string_view> const words = Tokenize(text);

    bool hasIntent = false;
    bool previousWasSize = false;
    std::optional<Token> pending;  // a role or a class still waiting for its other half
    auto flushPending = [&announce, &pending]()
    {
        if (!pending)
            return;

        RecruitSlot slot;
        if (pending->kind == TokenKind::Role)
            slot.role = pending->role;
        else
            slot.classId = pending->classId;

        AddSlot(announce.slots, slot);
        pending.reset();
    };

    for (size_t i = 0; i < words.size(); ++i)
    {
        std::string_view const word = words[i];
        Token token;
        if (word == "looking")
        {
            if (IsRecruitingLookingFor(words, i))
                token.kind = TokenKind::Intent;
        }
        else if (word == "death" && i + 1 < words.size() && words[i + 1] == "knight")
        {
            token.kind = TokenKind::Class;
            token.classId = CLASS_DEATH_KNIGHT;
            ++i;
        }
        else
            token = Classify(word, previousWasSize);

        previousWasSize = token.kind == TokenKind::Size;

        switch (token.kind)
        {
            case TokenKind::Other:
                break;
            case TokenKind::Role:
            case TokenKind::Class:
                // a role next to a class, in either order, is one slot: "hunter dps", "tank dk"
                if (pending && pending->kind != token.kind)
                {
                    RecruitSlot slot;
                    slot.role = token.kind == TokenKind::Role ? token.role : pending->role;
                    slot.classId = token.kind == TokenKind::Class ? token.classId : pending->classId;
                    AddSlot(announce.slots, slot);
                    pending.reset();
                    break;
                }

                flushPending();
                pending = token;
                break;
            case TokenKind::Spec:
            {
                flushPending();
                RecruitSlot slot;
                slot.role = token.role;
                slot.classId = token.classId;
                AddSlot(announce.slots, slot);
                break;
            }
            case TokenKind::Intent:
                hasIntent = true;
                flushPending();
                break;
            case TokenKind::Instance:
                if (announce.instance.empty())
                {
                    announce.instance = token.instance;
                    announce.heroic = announce.heroic || token.heroic;
                }

                flushPending();
                break;
            case TokenKind::Size:
                if (!announce.size)
                    announce.size = token.size;

                flushPending();
                break;
            case TokenKind::Heroic:
                announce.heroic = true;
                flushPending();
                break;
            case TokenKind::Cut:
                flushPending();
                break;
        }
    }

    flushPending();

    if (!hasIntent)
        return std::nullopt;

    // a class with no role is only a recruitment when a raid is named: "need a mage for portal" is not
    if (announce.instance.empty())
        std::erase_if(announce.slots, [](RecruitSlot const& slot) { return !slot.role; });

    if (announce.slots.empty())
        return std::nullopt;

    return announce;
}
