import { describe, expect, it } from 'vitest';
import { assetsByOS, classifyAsset, megabytes, primaryAssets, versionOf, type Release } from '../../src/lib/releases';
import fallback from '../../src/data/releases.fallback.json';

const asset = (name: string, size = 1_000_000) => ({ name, size, url: `https://example.test/${name}` });

describe('classifyAsset', () => {
  it('recognises every file a release ships, under each name the app has had', () => {
    for (const p of ['heap-v0.7.2', 'lowkey-v0.8.0', 'tabless-v0.9.0']) {
      expect(classifyAsset(asset(`${p}-windows-setup.exe`))).toMatchObject({ os: 'windows', kind: 'installer', rank: 0 });
      expect(classifyAsset(asset(`${p}-windows-portable.zip`))).toMatchObject({ os: 'windows', kind: 'portable', rank: 1 });
      expect(classifyAsset(asset(`${p}-macos.dmg`))).toMatchObject({ os: 'macos', kind: 'dmg' });
      expect(classifyAsset(asset(`${p}-linux-x86_64.AppImage`))).toMatchObject({ os: 'linux', kind: 'appimage' });
    }
  });

  it('ignores files that are not downloads', () => {
    expect(classifyAsset(asset('SHA256SUMS'))).toBeNull();
    expect(classifyAsset(asset('lowkey-v0.8.0-sbom.spdx.json'))).toBeNull();
    // Qt 6.4 builds shipped up to 0.5.1; the UI does not run on them.
    expect(classifyAsset(asset('heap-v0.5.1-linux-amd64.deb'))).toBeNull();
    expect(classifyAsset(asset('heap-v0.5.1-linux-x86_64.tar.gz'))).toBeNull();
  });
});

describe('the committed release snapshot', () => {
  const latest = (fallback as Release[]).find((r) => !r.prerelease)!;

  it('has a file for every platform, the installer first on Windows', () => {
    const by = assetsByOS(latest);
    expect(by.windows.map((a) => a.kind)).toEqual(['installer', 'portable']);
    expect(by.macos.map((a) => a.kind)).toEqual(['dmg']);
    expect(by.linux.map((a) => a.kind)).toEqual(['appimage']);
    expect(Object.keys(primaryAssets(latest)).sort()).toEqual(['linux', 'macos', 'windows']);
  });

  it('offers the lowkey-* files, not the heap-* copies kept for the 0.7.x updater', () => {
    const by = assetsByOS(latest);
    for (const list of Object.values(by)) for (const a of list) expect(a.name).toMatch(/^lowkey-/);
  });

  // The snapshot predates the rename to Tabless; GitHub redirects the old repository's links.
  it('points every download at the repository, under its current or its previous name', () => {
    for (const a of latest.assets) expect(a.url).toMatch(/^https:\/\/github\.com\/sectapunterx\/(tabless|lowkey)\/releases\/download\//);
  });
});

describe('a release carrying the feature build', () => {
  const stable = ['windows-setup.exe', 'windows-portable.zip', 'macos.dmg', 'linux-x86_64.AppImage'].map((s) => asset(`tabless-v0.9.2-${s}`));
  const feature = ['windows-portable.zip', 'macos.dmg', 'linux-x86_64.AppImage'].map((s) => asset(`tabless-v0.9.2-feature-a29c58a-${s}`));
  const release: Release = { tag: 'v0.9.2', name: 'Tabless v0.9.2', date: '2026-10-14T00:00:00Z', body: '', url: '', prerelease: false,
    assets: [...feature, ...stable, asset('SHA256SUMS-feature')] };

  it('offers only the release files, never the feature build', () => {
    const by = assetsByOS(release);
    const names = Object.values(by).flat().map((a) => a.name);
    expect(names).toHaveLength(4);
    for (const n of names) expect(n).not.toMatch(/-feature-/);
    expect(primaryAssets(release).windows?.name).toBe('tabless-v0.9.2-windows-setup.exe');
  });
});

describe('a release made during a rename', () => {
  const files = (prefix: string) =>
    ['windows-setup.exe', 'windows-portable.zip', 'macos.dmg', 'linux-x86_64.AppImage'].map((s) => asset(`${prefix}-v0.9.0-${s}`));
  const release = (assets: ReturnType<typeof asset>[]): Release => ({ tag: 'v0.9.0', name: 'v0.9.0', date: '2026-10-20T00:00:00Z', body: '', url: '', prerelease: false, assets });

  it('offers only the files under the newest name', () => {
    const by = assetsByOS(release([...files('heap'), ...files('lowkey'), ...files('tabless')]));
    const names = Object.values(by).flat().map((a) => a.name);
    expect(names).toHaveLength(4);
    for (const n of names) expect(n).toMatch(/^tabless-/);
  });

  it('still offers the lowkey-* files of a release that has no tabless-* ones', () => {
    const by = assetsByOS(release([...files('heap'), ...files('lowkey')]));
    for (const n of Object.values(by).flat().map((a) => a.name)) expect(n).toMatch(/^lowkey-/);
  });
});

describe('megabytes / versionOf', () => {
  it('formats like the download page shows it', () => {
    expect(megabytes(39_715_428)).toBe('39.7');
    expect(megabytes(50_526_712)).toBe('50.5');
    expect(versionOf('v0.8.0')).toBe('0.8.0');
  });
});
