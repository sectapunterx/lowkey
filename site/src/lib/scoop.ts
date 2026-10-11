// Build-time only. The Scoop bucket is bucket/tabless.json in this repository (lowkey.json and
// heap.json are the same manifest under the old names); the download page
// offers it only while it installs the release the page is about, so it never promises an older
// version than the buttons next to it.
import { readFileSync } from 'node:fs';
import { resolve } from 'node:path';

export function scoopVersion(): string | null {
  try {
    const manifest = JSON.parse(readFileSync(resolve(process.cwd(), '../bucket/tabless.json'), 'utf8')) as { version?: string };
    return manifest.version ?? null;
  } catch {
    return null;
  }
}
