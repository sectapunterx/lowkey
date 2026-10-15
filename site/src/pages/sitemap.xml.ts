// /sitemap.xml: every page in both languages, each with its hreflang twin, for search engines.
// A new page goes into PATHS; robots.txt (public/) points here.
import type { APIRoute } from 'astro';
import { topics } from '../data/topics';
import { url } from '../lib/site';

const PATHS = ['', 'download/', ...topics.en.map((t) => `${t.slug}/`)];

export const GET: APIRoute = ({ site }) => {
  const abs = (p: string) => new URL(url(p), site).href;
  const entries = PATHS.flatMap((p) =>
    (['', 'ru/'] as const).map((prefix) => {
      const alt = (['', 'ru/'] as const)
        .map((l) => `    <xhtml:link rel="alternate" hreflang="${l ? 'ru' : 'en'}" href="${abs(l + p)}"/>`)
        .join('\n');
      return `  <url>\n    <loc>${abs(prefix + p)}</loc>\n${alt}\n  </url>`;
    }),
  );
  const xml = `<?xml version="1.0" encoding="UTF-8"?>
<urlset xmlns="http://www.sitemaps.org/schemas/sitemap/0.9" xmlns:xhtml="http://www.w3.org/1999/xhtml">
${entries.join('\n')}
</urlset>
`;
  return new Response(xml, { headers: { 'Content-Type': 'application/xml; charset=utf-8' } });
};
