// The fonts heap ships inside the binary (platform/BundledFonts, qrc :/fonts):
// every face registers, the families come back under their plain names, and
// each weight the UI asks for (Inter and Golos Text 400/500/600, JetBrains Mono 400/500)
// resolves to that face rather than a synthesised or system one. Static faces
// with legacy naming are what makes this hold on every font backend — a
// variable font or "Golos Text Medium" as its own family would fail here.

#include "platform/BundledFonts.h"

#include <QDir>
#include <QFile>
#include <QFont>
#include <QFontDatabase>
#include <QFontInfo>
#include <QGuiApplication>
#include <QRawFont>

#include <gtest/gtest.h>

using heap::platform::kDisplayFontFamily;
using heap::platform::kMonoFontFamily;
using heap::platform::kOnestFontFamily;
using heap::platform::kUiFontFamily;

namespace {

QString ui() {
  return QString::fromLatin1(kUiFontFamily);
}

QString display() {
  return QString::fromLatin1(kDisplayFontFamily);
}

QString mono() {
  return QString::fromLatin1(kMonoFontFamily);
}

QString onest() {
  return QString::fromLatin1(kOnestFontFamily);
}

// What registerBundledFonts() returned in main(), before any test ran.
int g_failedFaces = -1;

}  // namespace

TEST(BundledFonts, AllFacesAreCompiledIn) {
  const QStringList files = QDir(QStringLiteral(":/fonts")).entryList({QStringLiteral("*.ttf")}, QDir::Files, QDir::Name);
  EXPECT_EQ(files,
            QStringList({QStringLiteral("GolosText-Medium.ttf"),
                         QStringLiteral("GolosText-Regular.ttf"),
                         QStringLiteral("GolosText-SemiBold.ttf"),
                         QStringLiteral("Inter-Medium.ttf"),
                         QStringLiteral("Inter-Regular.ttf"),
                         QStringLiteral("Inter-SemiBold.ttf"),
                         QStringLiteral("JetBrainsMono-Medium.ttf"),
                         QStringLiteral("JetBrainsMono-Regular.ttf"),
                         QStringLiteral("Onest-Medium.ttf"),
                         QStringLiteral("Onest-Regular.ttf"),
                         QStringLiteral("Onest-SemiBold.ttf")}));
}

TEST(BundledFonts, EveryFaceRegisters) {
  EXPECT_EQ(g_failedFaces, 0);
}

TEST(BundledFonts, FamiliesAreRegistered) {
  const QStringList families = QFontDatabase::families();
  EXPECT_TRUE(families.contains(ui())) << qPrintable(families.join(QStringLiteral(", ")));
  EXPECT_TRUE(families.contains(mono()));
  EXPECT_TRUE(families.contains(display()));
  EXPECT_TRUE(families.contains(onest()));
  // One family per font: a weight never shows up as a family of its own.
  EXPECT_FALSE(families.contains(ui() + QStringLiteral(" Medium")));
  EXPECT_FALSE(families.contains(ui() + QStringLiteral(" SemiBold")));
  EXPECT_FALSE(families.contains(mono() + QStringLiteral(" Medium")));
}

// The bundled faces never carry the upstream names. With JetBrains Mono
// installed on Windows, a bundled face sharing the name was mixed with the
// installed one (shaped against one file's glyph ids, drawn from the other's)
// and text in it came out as random glyphs (0.6.0).
TEST(BundledFonts, NamesNeverCollideWithAnInstalledCopy) {
  EXPECT_TRUE(ui().startsWith(QStringLiteral("Tabless ")));
  EXPECT_TRUE(mono().startsWith(QStringLiteral("Tabless ")));
  EXPECT_TRUE(display().startsWith(QStringLiteral("Tabless ")));
  EXPECT_TRUE(onest().startsWith(QStringLiteral("Tabless ")));
  const QDir dir(QStringLiteral(":/fonts"));
  for(const QString& file : dir.entryList({QStringLiteral("*.ttf")}, QDir::Files)) {
    QFile f(dir.filePath(file));
    ASSERT_TRUE(f.open(QIODevice::ReadOnly));
    const QRawFont raw(f.readAll(), 12);
    ASSERT_TRUE(raw.isValid()) << qPrintable(file);
    EXPECT_TRUE(raw.familyName() == ui() || raw.familyName() == display() || raw.familyName() == mono() || raw.familyName() == onest())
        << qPrintable(file + QStringLiteral(": ") + raw.familyName());
  }
}

TEST(BundledFonts, StylesAreTheShippedWeights) {
  for(const QString& family : {ui(), display(), onest()}) {
    const QStringList styles = QFontDatabase::styles(family);
    EXPECT_TRUE(styles.contains(QStringLiteral("Regular"))) << qPrintable(styles.join(QStringLiteral(", ")));
    EXPECT_TRUE(styles.contains(QStringLiteral("Medium")));
    EXPECT_TRUE(styles.contains(QStringLiteral("SemiBold")));
  }
  const QStringList monoStyles = QFontDatabase::styles(mono());
  EXPECT_TRUE(monoStyles.contains(QStringLiteral("Regular"))) << qPrintable(monoStyles.join(QStringLiteral(", ")));
  EXPECT_TRUE(monoStyles.contains(QStringLiteral("Medium")));
  // Not QFontDatabase::isFixedPitch: fontconfig counts JetBrains Mono as "dual"
  // spacing (a few glyphs are two cells wide), so it is false on Linux for the
  // upstream font too. What the UI relies on is that text characters share one
  // advance.
  QFont monoFont(mono());
  monoFont.setPixelSize(13);
  const QRawFont raw = QRawFont::fromFont(monoFont);
  ASSERT_TRUE(raw.isValid());
  const QString sample = QStringLiteral("iWm0O1l.-_APP-108 Жыё");
  const QList<QPointF> advances = raw.advancesForGlyphIndexes(raw.glyphIndexesForString(sample));
  for(const QPointF& a : advances) {
    EXPECT_DOUBLE_EQ(a.x(), advances.first().x());
  }
}

TEST(BundledFonts, EachWeightResolvesToItsFace) {
  const QList<QPair<QString, QFont::Weight>> wanted = {
      {ui(), QFont::Normal},
      {ui(), QFont::Medium},
      {ui(), QFont::DemiBold},
      {display(), QFont::Normal},
      {display(), QFont::Medium},
      {display(), QFont::DemiBold},
      {mono(), QFont::Normal},
      {mono(), QFont::Medium},
  };
  for(const auto& [family, weight] : wanted) {
    QFont font(family);
    font.setWeight(weight);
    const QFontInfo info(font);
    EXPECT_EQ(info.family(), family) << weight;
    EXPECT_EQ(info.weight(), weight) << qPrintable(family);
  }
  EXPECT_TRUE(heap::platform::missingBundledFonts().isEmpty())
      << qPrintable(heap::platform::missingBundledFonts().join(QStringLiteral(", ")));
}

// tools/gen_bundled_fonts.py cuts both fonts smaller than upstream by raising
// unitsPerEm (1000 / scale), so the Theme.fs* scale, set for Segoe UI and
// Consolas, keeps its size: at 13px the x-height is Segoe's (0.50 em) and
// Consolas' (0.49 em) within a few percent rather than 6% and 12% over.
TEST(BundledFonts, SizedToTheSegoeAndConsolasScale) {
  const QDir dir(QStringLiteral(":/fonts"));
  for(const QString& file : dir.entryList({QStringLiteral("*.ttf")}, QDir::Files)) {
    QFile f(dir.filePath(file));
    ASSERT_TRUE(f.open(QIODevice::ReadOnly));
    const QRawFont raw(f.readAll(), 13);
    ASSERT_TRUE(raw.isValid()) << qPrintable(file);
    const bool isMono = file.startsWith(QStringLiteral("JetBrainsMono"));
    // Inter (2816 upstream) at 0.91, Golos Text (1000) at 0.93, Onest (1000) at 0.94,
    // JetBrains Mono (1000) at 0.88.
    const double upm = isMono                                     ? 1136.0
                       : file.startsWith(QStringLiteral("Inter")) ? 3095.0
                       : file.startsWith(QStringLiteral("Onest")) ? 1064.0
                                                                  : 1075.0;
    EXPECT_DOUBLE_EQ(raw.unitsPerEm(), upm) << qPrintable(file);
    EXPECT_LE(raw.xHeight(), isMono ? 13 * 0.49 * 1.05 : 13 * 0.50 * 1.01) << qPrintable(file);
    // Smaller, not shrunk out of shape: still well above a 13px Segoe UI's
    // x-height of ~5.5px.
    EXPECT_GE(raw.xHeight(), 13 * 0.45) << qPrintable(file);
  }
}

TEST(BundledFonts, CoverCyrillic) {
  // The interface is Russian as well as English.
  for(const QString& family : {ui(), display(), mono(), onest()}) {
    const QRawFont raw = QRawFont::fromFont(QFont(family));
    ASSERT_TRUE(raw.isValid()) << qPrintable(family);
    EXPECT_TRUE(raw.supportsCharacter(QChar(0x0416))) << qPrintable(family);  // Ж
    EXPECT_TRUE(raw.supportsCharacter(QChar(0x0451))) << qPrintable(family);  // ё
  }
}

// The ↵ in every key hint is a glyph tools/fonts/add_return_glyph.py adds.
// Its first version grew the glyph count but left Golos's vmtx a glyph short:
// FreeType (these tests, offscreen) still draws it, DirectWrite (the app on
// Windows) draws an empty advance. So check the tables, not a rendering.
namespace {

quint32 be32(const QByteArray& data, qsizetype at) {
  const auto* p = reinterpret_cast<const uchar*>(data.constData() + at);
  return (quint32(p[0]) << 24) | (quint32(p[1]) << 16) | (quint32(p[2]) << 8) | quint32(p[3]);
}

quint16 be16(const QByteArray& data, qsizetype at) {
  const auto* p = reinterpret_cast<const uchar*>(data.constData() + at);
  return quint16((p[0] << 8) | p[1]);
}

// Offset and length of an sfnt table, or {-1, 0}.
std::pair<qsizetype, quint32> table(const QByteArray& data, const char* tag) {
  const int count = be16(data, 4);
  for(int i = 0; i < count; ++i) {
    const qsizetype rec = 12 + i * 16;
    if(data.mid(rec, 4) == QByteArray(tag, 4)) {
      return {be32(data, rec + 8), be32(data, rec + 12)};
    }
  }
  return {-1, 0};
}

}  // namespace

TEST(BundledFonts, MetricsTablesCoverEveryGlyph) {
  const QDir dir(QStringLiteral(":/fonts"));
  for(const QString& file : dir.entryList({QStringLiteral("*.ttf")}, QDir::Files)) {
    QFile f(dir.filePath(file));
    ASSERT_TRUE(f.open(QIODevice::ReadOnly));
    const QByteArray data = f.readAll();
    const auto [maxp, maxpLen] = table(data, "maxp");
    ASSERT_GE(maxp, 0) << qPrintable(file);
    const int glyphs = be16(data, maxp + 4);
    for(const auto& [head, metrics] : {std::pair{"hhea", "hmtx"}, std::pair{"vhea", "vmtx"}}) {
      const auto [h, hLen] = table(data, head);
      if(h < 0) {
        continue;  // JetBrains Mono has no vertical metrics
      }
      const int longMetrics = be16(data, h + 34);
      const auto [m, mLen] = table(data, metrics);
      ASSERT_GE(m, 0) << qPrintable(file) << " " << metrics;
      EXPECT_GE(mLen, quint32(longMetrics * 4 + (glyphs - longMetrics) * 2)) << qPrintable(file) << " " << metrics;
    }
    const QRawFont raw(data, 13);
    EXPECT_TRUE(raw.supportsCharacter(QChar(0x21B5))) << qPrintable(file);
  }
}

int main(int argc, char** argv) {
  qputenv("QT_QPA_PLATFORM", "offscreen");
  const QGuiApplication app(argc, argv);
  g_failedFaces = heap::platform::registerBundledFonts();
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
