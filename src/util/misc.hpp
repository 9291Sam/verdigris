#ifndef SRC_UTIL_MISC_HPP
#define SRC_UTIL_MISC_HPP

#include <algorithm>
#include <array>
#include <atomic>
#include <bit>
#include <cmath>
#include <concepts>
#include <type_traits>

namespace util
{

    constexpr std::array<std::uint64_t, 256> CRCConstants = {
        0xC233'ADA5'72A6'6021, 0xDE7A'C5D4'5416'A75F, 0x2933'5F47'6D7A'E1B9,
        0x3F54'442E'EE36'FD58, 0xCE02'B645'CC60'265C, 0xE549'721D'B029'1B32,
        0xE69B'E529'DE2C'99A1, 0x8317'2C61'EA61'C6DF, 0x9BD2'D144'C5AA'1A98,
        0x1CA8'3050'79C3'5D06, 0x9994'1789'AC73'8191, 0x00DD'52EF'DFA6'B361,
        0x7977'914B'9673'E238, 0x10F7'684D'E32F'BE3C, 0x27A6'E37C'3864'3726,
        0x0587'1112'BBC0'D540, 0x0D70'BD14'7A4B'416B, 0x6AE5'98C0'3DF5'FC45,
        0x5E07'2249'85CA'EBDB, 0x5A54'1E82'9DC6'5D18, 0x7B65'8ED7'258F'29DE,
        0xE372'E7F9'2774'6A3C, 0x5EB5'5EB5'5EB5'5EB5, 0x0516'88FB'2B6A'B9FD,
        0x80A2'86F1'538D'3043, 0x7C9B'7BFA'55D4'92AC, 0x6F13'FC07'DAB0'15BA,
        0x6A10'624C'0DD3'67E0, 0x5740'D8F3'5428'CBC2, 0x75FF'6297'95D2'0F0F,
        0x7DC9'75A9'7F01'F0B8, 0x9241'F37A'A234'DDA0, 0xDD9E'777C'FA78'9C75,
        0x5298'3144'47A3'29F3, 0xBFAC'EFB6'D96D'366D, 0x98CC'CC53'79CF'87AE,
        0x7417'0D36'5A33'9961, 0x0491'6A99'B831'E4E7, 0x2267'C05B'6F8D'3CA7,
        0xFF75'9D3F'585E'8234, 0xBB54'AA30'D159'E07B, 0x64D5'F92C'92A4'7C02,
        0xF2E1'AD5E'CFA9'1485, 0x9D2F'5E5F'2B35'99BF, 0x19D9'2738'C21D'B670,
        0x47DF'D10C'292E'43A5, 0x3A83'5EB5'5EB5'1F84, 0xFCAB'3816'408D'06B9,
        0xAA4E'63B4'C0C5'E15B, 0xB018'566D'D88C'0FE9, 0x6B0F'0C25'DCDB'3E46,
        0x6B02'F888'0A21'1FEA, 0x6B2E'08A8'03C3'99B7, 0x600B'093B'E9AB'94D5,
        0x381B'486D'1501'4171, 0x285C'C47A'E64F'ECE0, 0x96D8'D6B5'7285'A31B,
        0x6A94'6AD7'9A49'9B7D, 0x5935'413B'9A43'B91A, 0x3045'B9FB'BBC5'3344,
        0x3801'D632'B709'D79E, 0xC3B5'691E'500A'3CCB, 0x9D49'1251'1071'2214,
        0x5145'CBD7'B5F5'6875, 0x8EF0'A0BB'0097'A713, 0x7488'1BB0'2DD1'5EE0,
        0x3DB3'D139'DB20'0D18, 0x9426'3A79'9510'2ADF, 0x5ABC'097C'0A03'0E73,
        0x20F1'0D8E'D66E'3932, 0x3A57'8DD9'BB7F'C499, 0xAD20'F347'3EEC'3A7B,
        0xA4E2'A6E0'BE49'77F6, 0xF4F2'7B53'1A06'BA44, 0xD2AE'21C1'B294'2AF6,
        0x1ECD'248C'9840'6ED7, 0x1B37'E6D1'3CD8'0F61, 0x480E'60F9'C7F3'B431,
        0x4F83'8413'08E9'5673, 0x4A51'08D9'EE5D'2E5E, 0x9E5B'79B5'2EE6'6C29,
        0xD72F'F0E5'E1B2'5789, 0x6456'EA4E'F71A'2E4B, 0x33FA'3D54'AFDF'4A10,
        0xCDEE'A9B6'DB00'E458, 0x87FB'11E7'F23D'E622, 0x665C'2F05'5159'AA74,
        0x4C7B'6CB3'63E9'5A6D, 0xD8D7'7F46'D359'552D, 0x8438'0EA6'4119'18EF,
        0xA858'3325'BEF2'8C04, 0x3198'38D6'5ED7'B7D8, 0xF1DF'4075'FBF8'E072,
        0x8143'EDD2'BFBA'3A07, 0x1C7B'E468'AE8E'3D51, 0x24E9'0564'81E7'0860,
        0x6269'C332'68EC'43C0, 0x9EAD'F4EA'46FB'AC7C, 0x4E00'1D65'BD92'FE38,
        0x89C5'034A'EB35'C3C1, 0x1981'53F4'2A46'992A, 0x398E'1BEE'1B83'4C30,
        0x608B'419E'10E5'4D4A, 0x61D3'DB5B'818D'73F7, 0xE474'D2CE'E3B9'176A,
        0xBC06'766B'50FF'507C, 0x6B53'3A7E'F077'6A63, 0x0CE1'CF11'380C'AC43,
        0x5BCA'DB7D'7C3D'B57E, 0x0B2B'7233'FCEB'3DF9, 0x8089'F536'FCC0'EEE7,
        0xF049'99D9'8833'9C74, 0x68CC'ECE1'43FE'721D, 0x7438'98A2'5FB1'33C5,
        0xC764'BA79'5AFB'D40A, 0xE843'F065'9312'3ABB, 0x645E'7F5C'8B54'3269,
        0xE33E'D1B0'D22F'9B66, 0x1356'387B'895A'A2F6, 0x8FF5'458C'C6A7'CEA8,
        0xBC09'D88E'6B74'748C, 0xD100'1660'8A26'5D8E, 0x7CE2'590F'776D'12CA,
        0x9FD3'A3F2'ED82'079C, 0xA746'2F3C'DBFE'E0E8, 0x93C7'2470'CC99'C369,
        0xF4F1'E164'DB51'A6B6, 0x0A9E'7378'E7CD'2F4A, 0x90E0'B9ED'FEBE'9913,
        0x0A76'1F03'F686'E8DA, 0x2EA8'E4DA'1494'7194, 0x2F2C'634A'9ED9'FA3B,
        0xB5E4'F2A9'64B4'ED9D, 0x700C'119D'9844'9D82, 0x0471'8FB6'5604'08E4,
        0x0BC8'6EC0'1FEE'2D7E, 0xE55F'86DD'BFDA'68D3, 0x4696'120C'D6A5'74C1,
        0xA1C2'662D'3A94'9317, 0xC1DB'664A'7186'D14D, 0xACA9'67D1'14BE'1F1C,
        0xDC73'96F8'B961'6F25, 0x371C'77CA'44E4'6102, 0x2B99'8721'3C3C'07A6,
        0x2B05'4D2D'B267'4B1C, 0xE730'0950'AA75'C557, 0x4F83'1103'C057'8564,
        0x06F9'5899'C034'A36A, 0x246E'443B'5C7B'50A1, 0x2425'F10E'0A09'A0CE,
        0x682F'5DE2'637F'8EB5, 0x989F'EE74'EA1D'FB04, 0xCF18'3CE1'689F'CD24,
        0x0590'7856'F5C1'9A1F, 0xD453'BC9F'B130'9A39, 0xC424'2496'6B7C'86CD,
        0x6A0F'9066'9A94'2C95, 0x738A'6EF6'1A3B'5076, 0xC405'C42D'33E5'5D50,
        0xDD09'9D37'8B16'C59C, 0xC9CA'EDBF'C5B0'BD71, 0xC7E4'DAEA'3991'0935,
        0x2C02'20EF'421B'83D0, 0x4BE7'B8CD'F8A0'CDCA, 0x24F0'84E9'FD6B'F466,
        0x640C'C61B'3AB0'65C4, 0x711E'8723'2934'A482, 0x3352'7F3B'4B25'97C1,
        0x07CF'98D4'F82A'DEE1, 0x5FE0'F137'790A'74AC, 0x564F'3D6E'1B83'2FD4,
        0xC58E'57BC'AD22'03C0, 0x3C3E'8D4B'2BD5'3B47, 0x16BD'58FC'60C2'0E30,
        0xC3D7'3B40'2448'AD61, 0x9129'1B6A'A865'4A91, 0x3B32'05AA'7EBC'5B94,
        0xDEE3'1DDF'FA57'1E5F, 0xCA87'39F2'1EF2'0600, 0x5E7A'DCF0'0F65'6680,
        0x14CC'54E4'16FC'67E9, 0x476F'EBB7'FA77'5B64, 0xDD0B'7FCC'E852'74EC,
        0x9881'75EA'F7C8'BE87, 0x51AD'F604'C0E5'C2EF, 0xC114'6C19'16AC'1DCE,
        0xAC8F'62E7'3A09'87C2, 0xCB5C'E212'848B'E575, 0xED24'CE54'8AFF'24CF,
        0x4130'2C7B'6CD5'E4F9, 0x55F6'396D'26BD'A116, 0x8D2A'C8D1'5886'2A91,
        0xB506'65CB'0A72'96D1, 0x051A'5D6C'2E7C'3447, 0x850E'A3DC'4810'8BA9,
        0x41EE'571B'8579'40F7, 0x4C2A'8EEE'D639'63E2, 0x7DBA'B64B'F70F'A564,
        0x686D'A36D'7CD3'0BFB, 0x40BD'11B5'364D'84EA, 0x0854'80EB'9C39'BEF2,
        0x38FC'8B01'C13D'DCD2, 0xB55C'6B77'B35A'FCFA, 0x5676'0784'0F76'7D10,
        0xF12E'4577'3ECA'775F, 0xB959'543C'17F0'C6A2, 0xAB3D'CEFE'F418'0C3D,
        0x34F1'2457'D5FB'06DF, 0x7845'21F7'9F5C'2A44, 0x25E0'A897'80DA'86DF,
        0x945A'197C'C40D'62DC, 0xB103'261F'CAD3'2C84, 0x5D50'0ABC'F4E4'6549,
        0x330A'70A2'1BAD'908A, 0xF2CC'640A'6AA5'0FBC, 0xCDDB'FDEC'4C14'AF65,
        0xB03C'3470'F732'3C7F, 0x6C97'8A8D'4319'28DA, 0x219F'E20A'8666'3029,
        0xE613'F743'02F2'89ED, 0x58D6'5E24'4096'A697, 0x907B'EEA7'4842'1C5E,
        0xB536'1CC5'D835'957B, 0xF5CE'1112'EE11'AE03, 0x4E75'FA38'5BD0'3022,
        0x7F8D'4947'845A'D30D, 0x67E9'2FDB'DE7E'55EC, 0x119C'FE34'57F1'849C,
        0x9639'9617'438A'DA3F, 0xD7C3'3304'EBCB'9A4C, 0x7B2D'E4CB'CA57'A1EB,
        0x9789'4418'AC83'FE46, 0x246F'886B'82AC'01C0, 0xB32E'9D34'16CE'BC91,
        0xA41B'3AE0'B7E5'9E1D, 0xDB5E'F530'FDF7'AABC, 0xC549'FD5A'982A'C854,
        0xCA4F'5F18'9E4D'DBF4, 0x4655'5189'6EB3'149F, 0xF1F9'F13E'8AE1'B862,
        0x187D'D7E7'0BB6'C204, 0x3CED'96E2'AE9A'D54F, 0x5AC3'FE5A'070E'A32D,
        0x79C8'58EB'55CF'9FFB, 0x6D26'F3D5'6DCE'E005, 0xAE36'B9F3'C49C'E8EE,
        0x305E'0CBD'F5CF'F05B, 0xD10C'0394'DE5A'74DB, 0xC1E6'CF1C'7E2D'D70C,
        0x199B'9724'13B6'3159, 0x5D57'9AFF'AB65'689C, 0xE343'E96D'4B1A'8C98,
        0xE197'51F3'0310'22E7, 0xB966'0FF1'13BF'1DE0, 0xC930'EA45'C754'05F5,
        0xC8D4'CF6B'9F88'40B3};

    [[noreturn]] void debugBreak();

    template<class T>
    concept Integer = requires {
        requires std::integral<T>;
        requires !std::floating_point<T>;
        requires !std::same_as<T, bool>;
        requires !std::is_pointer_v<T>;
    };

    template<class T>
    concept UnsignedInteger = requires {
        requires Integer<T>;
        requires std::is_unsigned_v<T>;
    };

    template<class T>
    concept Arithmetic = requires {
        requires std::integral<T> || std::floating_point<T>;
        requires !std::same_as<T, bool>;
        requires !std::is_pointer_v<T>;
    };

    template<class T>
    class AtomicUniquePtr
    {
    public:
        explicit AtomicUniquePtr() noexcept
            : owned_t {nullptr}
        {}

        template<class... Args>
        explicit AtomicUniquePtr(Args&&... args)
            requires std::is_constructible_v<T, Args...>
            : owned_t {new T {std::forward<Args>(args)...}}
        {}

        explicit AtomicUniquePtr(T&& t) // NOLINT
            requires std::is_move_constructible_v<T>
            : owned_t {new T {std::move<T>(t)}}
        {}

        ~AtomicUniquePtr()
        {
            delete /* NOLINT */ this->owned_t.exchange(
                nullptr, std::memory_order_acq_rel);
        }

        AtomicUniquePtr(const AtomicUniquePtr&) = delete;
        AtomicUniquePtr(AtomicUniquePtr&& other) noexcept
            : owned_t {nullptr}
        {
            this->owned_t.store(
                other.owned_t.exchange(nullptr, std::memory_order_acq_rel),
                std::memory_order_acq_rel);
        }
        AtomicUniquePtr& operator= (const AtomicUniquePtr&) = delete;
        AtomicUniquePtr& operator= (AtomicUniquePtr&& other) noexcept
        {
            delete /* NOLINT */ this->owned_t.exchange(
                nullptr, std::memory_order_acq_rel);

            this->owned_t =
                other.owned_t.exchange(nullptr, std::memory_order_acq_rel);
        }

        // Primary interaction method
        operator std::atomic<T*>& () const // NOLINT: implicit
        {
            return this->owned_t;
        }

        [[nodiscard]] T* leak() noexcept
        {
            return this->owned_t.exchange(nullptr, std::memory_order_acq_rel);
        }

        void destroy() noexcept
        {
            delete /* NOLINT */ this->owned_t.exchange(
                nullptr, std::memory_order_acq_rel);
        }

    private:
        std::atomic<T*> owned_t;
    };

    template<UnsignedInteger I>
    [[nodiscard]] constexpr inline I crc(I input) noexcept
    {
        const std::array<std::uint8_t, sizeof(I)> data =
            std::bit_cast<std::array<std::uint8_t, 8>>(input);

        I out = ~I {0};

        for (std::size_t i = 0; i < 8 * sizeof(i); i++) // NOLINT
        {
            out =
                (out >> 8) ^ CRCConstants[(out ^ data[i % 8]) & 0xFF]; // NOLINT
        }

        return ~out;
    }

    template<Integer I>
    constexpr inline I log2(I number)
    {
        return static_cast<I>(std::bit_width(number) - 1);
    }

    template<class Func>
    using Fn = Func*;

    template<Integer I>
    constexpr inline I exp(I base, I exp)
    {
        constexpr I Zero = static_cast<I>(0);
        constexpr I One  = static_cast<I>(1);

        switch (exp)
        {
        case Zero:
            return One;
        case One:
            return base;
        default:
            return base * ::util::exp(base, exp - 1);
        }
    }

    template<std::floating_point F>
    constexpr inline std::uint8_t convertLinearToSRGB(F value) noexcept
    {
        const F ClampedValue =
            std::clamp<F>(value, static_cast<F>(0.0), static_cast<F>(1.0));

        return static_cast<uint8_t>(
            255.0f * std::pow(ClampedValue, 1.0f / 2.4f));
    }

    constexpr inline float convertSRGBToLinear(std::uint8_t integer) noexcept
    {
        const float IntegerAsFloatNormalized =
            static_cast<float>(integer)
            / static_cast<float>(std::numeric_limits<std::uint8_t>::max());

        return std::pow(IntegerAsFloatNormalized, 2.4f);
    }
} // namespace util

#endif // SRC_UTIL_MISC_HPP
