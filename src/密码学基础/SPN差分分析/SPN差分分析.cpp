#include <algorithm>
#include <array>
#include <cstdint>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <random>
#include <vector>

using namespace std;

/**
 * 16 位 SPN：
 * - 4 轮
 * - 4 个 4-bit S-box
 * - 固定 16 位 P-box
 */

namespace
{
    constexpr array<uint8_t, 16> SBOX = {
        0xE, 0x4, 0xD, 0x1,
        0x2, 0xF, 0xB, 0x8,
        0x3, 0xA, 0x6, 0xC,
        0x5, 0x9, 0x0, 0x7};

    array<uint8_t, 16> SBOX_INV{};

    constexpr array<int, 16> PBOX = {
        0, 4, 8, 12,
        1, 5, 9, 13,
        2, 6, 10, 14,
        3, 7, 11, 15};

    array<int, 16> PBOX_INV{};

    array<uint16_t, 5> ROUND_KEYS{};

    mt19937 RNG(static_cast<unsigned>(time(nullptr)));
    uniform_int_distribution<int> U16(0, 0xFFFF);

    inline uint8_t nibbleFromHigh(uint16_t value, int position)
    {
        return static_cast<uint8_t>((value >> (16 - position * 4)) & 0xF);
    }

    inline uint16_t sboxLayer(uint16_t state)
    {
        uint16_t out = 0;
        for (int i = 0; i < 4; ++i)
        {
            uint8_t nibble = static_cast<uint8_t>((state >> (i * 4)) & 0xF);
            out |= static_cast<uint16_t>(SBOX[nibble]) << (i * 4);
        }
        return out;
    }

    inline uint16_t sboxLayerInv(uint16_t state)
    {
        uint16_t out = 0;
        for (int i = 0; i < 4; ++i)
        {
            uint8_t nibble = static_cast<uint8_t>((state >> (i * 4)) & 0xF);
            out |= static_cast<uint16_t>(SBOX_INV[nibble]) << (i * 4);
        }
        return out;
    }

    inline uint16_t pboxLayer(uint16_t state)
    {
        uint16_t out = 0;
        for (int i = 0; i < 16; ++i)
        {
            out |= static_cast<uint16_t>(((state >> i) & 1U) << PBOX[i]);
        }
        return out;
    }

    inline uint16_t pboxLayerInv(uint16_t state)
    {
        uint16_t out = 0;
        for (int i = 0; i < 16; ++i)
        {
            out |= static_cast<uint16_t>(((state >> i) & 1U) << PBOX_INV[i]);
        }
        return out;
    }

    struct PlainCipherPair
    {
        uint16_t plaintext;
        uint16_t ciphertext;
    };

    struct DifferentialQuad
    {
        uint16_t plaintext;
        uint16_t ciphertext;
        uint16_t plaintextStar;
        uint16_t ciphertextStar;
    };

    struct KeyCandidate
    {
        uint8_t k2;
        uint8_t k4;
        int hits;
        double score;
    };
} // namespace

void initTables()
{
    for (int i = 0; i < 16; ++i)
    {
        SBOX_INV[SBOX[i]] = static_cast<uint8_t>(i);
        PBOX_INV[PBOX[i]] = i;
    }
}

void generateRandomKeys()
{
    for (auto &key : ROUND_KEYS)
    {
        key = static_cast<uint16_t>(U16(RNG));
    }
}

uint16_t encrypt(uint16_t plaintext)
{
    uint16_t state = plaintext;
    for (int round = 0; round < 3; ++round)
    {
        state ^= ROUND_KEYS[round];
        state = sboxLayer(state);
        state = pboxLayer(state);
    }

    state ^= ROUND_KEYS[3];
    state = sboxLayer(state);
    state ^= ROUND_KEYS[4];
    return state;
}

uint16_t decrypt(uint16_t ciphertext)
{
    uint16_t state = ciphertext;
    state ^= ROUND_KEYS[4];
    state = sboxLayerInv(state);
    state ^= ROUND_KEYS[3];

    for (int round = 2; round >= 0; --round)
    {
        state = pboxLayerInv(state);
        state = sboxLayerInv(state);
        state ^= ROUND_KEYS[round];
    }
    return state;
}

vector<DifferentialQuad> generateDifferentialPairs(size_t count, uint16_t inputDiff)
{
    vector<DifferentialQuad> quads;
    quads.reserve(count);
    for (size_t i = 0; i < count; ++i)
    {
        uint16_t plaintext = static_cast<uint16_t>(U16(RNG));
        uint16_t plaintextStar = static_cast<uint16_t>(plaintext ^ inputDiff);
        uint16_t ciphertext = encrypt(plaintext);
        uint16_t ciphertextStar = encrypt(plaintextStar);
        quads.push_back({plaintext, ciphertext, plaintextStar, ciphertextStar});
    }
    return quads;
}

void printTopCandidates(const vector<KeyCandidate> &candidates, int topN, const string &title)
{
    cout << title << '\n';
    cout << left << setw(10) << "k2"
         << setw(10) << "k4"
         << setw(12) << "hits"
         << setw(12) << "score" << '\n';

    for (int i = 0; i < min(topN, static_cast<int>(candidates.size())); ++i)
    {
        cout << hex << showbase
             << left << setw(10) << static_cast<int>(candidates[i].k2)
             << setw(10) << static_cast<int>(candidates[i].k4)
             << nouppercase << dec
             << setw(12) << candidates[i].hits
             << setw(12) << fixed << setprecision(6) << candidates[i].score
             << '\n';
    }
}

vector<KeyCandidate> buildDifferentialCandidates(const vector<DifferentialQuad> &quads, uint8_t expectedDiff = 0x6)
{
    array<array<int, 16>, 16> counts{};

    for (const auto &quad : quads)
    {
        uint8_t c2 = nibbleFromHigh(quad.ciphertext, 2);
        uint8_t c4 = nibbleFromHigh(quad.ciphertext, 4);
        uint8_t c2Star = nibbleFromHigh(quad.ciphertextStar, 2);
        uint8_t c4Star = nibbleFromHigh(quad.ciphertextStar, 4);

        for (int k2 = 0; k2 < 16; ++k2)
        {
            uint8_t u2 = SBOX_INV[c2 ^ k2];
            uint8_t u2Star = SBOX_INV[c2Star ^ k2];
            uint8_t diff2 = static_cast<uint8_t>(u2 ^ u2Star);

            for (int k4 = 0; k4 < 16; ++k4)
            {
                uint8_t u4 = SBOX_INV[c4 ^ k4];
                uint8_t u4Star = SBOX_INV[c4Star ^ k4];
                uint8_t diff4 = static_cast<uint8_t>(u4 ^ u4Star);

                if (diff2 == expectedDiff && diff4 == expectedDiff)
                {
                    ++counts[k2][k4];
                }
            }
        }
    }

    vector<KeyCandidate> candidates;
    candidates.reserve(256);
    for (int k2 = 0; k2 < 16; ++k2)
    {
        for (int k4 = 0; k4 < 16; ++k4)
        {
            int hits = counts[k2][k4];
            candidates.push_back({static_cast<uint8_t>(k2), static_cast<uint8_t>(k4), hits,
                                  static_cast<double>(hits) / max<size_t>(1, quads.size())});
        }
    }

    sort(candidates.begin(), candidates.end(), [](const KeyCandidate &a, const KeyCandidate &b)
         {
        if (a.hits != b.hits)
        {
            return a.hits > b.hits;
        }
        return a.score > b.score; });

    return candidates;
}

int candidateRank(const vector<KeyCandidate> &candidates, uint8_t realK2, uint8_t realK4)
{
    for (size_t i = 0; i < candidates.size(); ++i)
    {
        if (candidates[i].k2 == realK2 && candidates[i].k4 == realK4)
        {
            return static_cast<int>(i) + 1;
        }
    }

    return static_cast<int>(candidates.size()) + 1;
}

void differentialAttack(const vector<DifferentialQuad> &quads, uint8_t expectedDiff = 0x6, int topN = 5)
{
    vector<KeyCandidate> candidates = buildDifferentialCandidates(quads, expectedDiff);

    cout << "\n=== 差分攻击 ===\n";
    cout << "样本数: " << quads.size() << '\n';
    cout << "目标差分: " << hex << showbase << static_cast<int>(expectedDiff)
         << nouppercase << dec << '\n';
    printTopCandidates(candidates, topN, "候选密钥排序:");
}

int main()
{
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    initTables();

    cout << "SPN 差分攻击演示\n";
    cout << "========================\n";

    const size_t differentialSampleCount = 6000;
    const uint16_t inputDiff = 0x0B00;
    const int trialCount = 100;

    generateRandomKeys();
    uint8_t demoK2 = nibbleFromHigh(ROUND_KEYS[4], 2);
    uint8_t demoK4 = nibbleFromHigh(ROUND_KEYS[4], 4);

    cout << "真实轮密钥 K4 的第 2、4 个 nibble(从高位开始计): 0x"
         << hex << static_cast<int>(demoK2)
         << " 0x" << static_cast<int>(demoK4)
         << dec << '\n';

    auto differentialQuads = generateDifferentialPairs(differentialSampleCount, inputDiff);
    differentialAttack(differentialQuads);

    int successCount = 0;
    long long rankSum = 0;

    for (int trial = 0; trial < trialCount; ++trial)
    {
        generateRandomKeys();
        uint8_t realK2 = nibbleFromHigh(ROUND_KEYS[4], 2);
        uint8_t realK4 = nibbleFromHigh(ROUND_KEYS[4], 4);

        auto quads = generateDifferentialPairs(differentialSampleCount, inputDiff);
        auto candidates = buildDifferentialCandidates(quads, 0x6);
        int rank = candidateRank(candidates, realK2, realK4);

        if (rank == 1)
        {
            ++successCount;
        }
        rankSum += rank;
    }

    double successRate = static_cast<double>(successCount) / trialCount;
    double averageRank = static_cast<double>(rankSum) / trialCount;

    cout << "\n=== 重复统计 ===\n";
    cout << "试验次数: " << trialCount << '\n';
    cout << "成功率(真实密钥排第1): " << fixed << setprecision(2) << successRate * 100.0 << "%\n";
    cout << "平均候选密钥排名: " << fixed << setprecision(2) << averageRank << '\n';

    return 0;
}