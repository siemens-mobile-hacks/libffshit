#include <ffshit/filesystem/hash.h>

#include <gtest/gtest.h>

using FULLFLASH::Filesystem::name_hash_utf16;
using FULLFLASH::Filesystem::name_hash_8bit;

// The hashes the phones keep next to these names in their directories: of the S75 v40 and the
// EL71 v41 for UTF-16 names, of the CX70 v56 and the SL65 v49 for 8-bit names. The uploads are
// what the emulated phones stored for files sent to them over OBEX.

TEST(NameHashTest, HashesUtf16NamesLikeTheNewSgoldFirmware) {
    EXPECT_EQ(name_hash_utf16(u"Misc"),                  0xCA77);
    EXPECT_EQ(name_hash_utf16(u"Pictures"),              0x805A);
    EXPECT_EQ(name_hash_utf16(u"00"),                    0xBF4C);
    EXPECT_EQ(name_hash_utf16(u"09"),                    0x3F55);
    EXPECT_EQ(name_hash_utf16(u"Marble Madness.jad"),    0xC26A);
    EXPECT_EQ(name_hash_utf16(u"Siemens wallpaper.jpg"), 0x107F);
    EXPECT_EQ(name_hash_utf16(u"small.bin"),             0xBB37);
    EXPECT_EQ(name_hash_utf16(u"big.bin"),               0x3D00);
    EXPECT_EQ(name_hash_utf16(u"newdir"),                0x6273);
}

TEST(NameHashTest, FoldsTheCaseOfUtf16Names) {
    EXPECT_EQ(name_hash_utf16(u"tmp"),                   0x8445);
    EXPECT_EQ(name_hash_utf16(u"Tmp"),                   0x8445);
    EXPECT_EQ(name_hash_utf16(u"Аркады"),                0xAEE9);
    EXPECT_EQ(name_hash_utf16(u"аркады"),                0xAEE9);
    EXPECT_EQ(name_hash_utf16(u"Логические"),            0xD999);
    EXPECT_EQ(name_hash_utf16(u"призрак.mid"),           0x693A);
    EXPECT_EQ(name_hash_utf16(u"будильник.mp3"),         0xAF7F);
}

TEST(NameHashTest, HashesEightBitNamesLikeTheSgoldFirmware) {
    EXPECT_EQ(name_hash_8bit("Misc"),                    0x14B8);
    EXPECT_EQ(name_hash_8bit("Voice memo"),              0xE07B);
    EXPECT_EQ(name_hash_8bit("main"),                    0x9E63);
    EXPECT_EQ(name_hash_8bit("Cfg"),                     0x52A9);
    EXPECT_EQ(name_hash_8bit("default.cfg"),             0xB2BA);
    EXPECT_EQ(name_hash_8bit("s100.bin"),                0x6C06);
    EXPECT_EQ(name_hash_8bit("s2200.bin"),               0xABDE);
}

TEST(NameHashTest, FoldsTheCaseOfEightBitNames) {
    EXPECT_EQ(name_hash_8bit("MISC"),                    name_hash_8bit("misc"));
    EXPECT_EQ(name_hash_8bit("Misc"),                    name_hash_8bit("mIsC"));
}
