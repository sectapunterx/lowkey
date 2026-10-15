cask "tabless" do
  version "0.9.4"
  sha256 "0f67f784502fa780087837bea0e51a9c9938f74c67755d54163a1e601f749010"

  url "https://github.com/sectapunterx/tabless/releases/download/v#{version}/tabless-v#{version}-macos.dmg"
  name "Tabless"
  desc "Developer's workday in one window: tracker tasks, meetings and notes"
  homepage "https://tabless.dev/"

  livecheck do
    url :url
    strategy :github_latest
  end

  depends_on arch: :arm64
  depends_on macos: ">= :monterey"

  app "Tabless.app"
  # The command line (tabless today / add / now) is the app's own executable.
  binary "#{appdir}/Tabless.app/Contents/MacOS/Tabless", target: "tabless"

  zap trash: "~/Library/Application Support/Tabless"

  caveats <<~EOS
    Tabless is not notarized yet. On the first launch macOS may refuse to open it:
    right-click Tabless in Applications and choose Open, or on macOS 15 and later
    open it once, then System Settings > Privacy & Security > Open Anyway.
    Every release ships SHA256SUMS next to the files.
  EOS
end
