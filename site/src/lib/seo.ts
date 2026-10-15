// Structured data (schema.org JSON-LD) for search engines and AI answers: what Tabless is, what it
// costs and what it runs on, plus the questions a page answers. Facts follow site/PROMISES.md.
import { GITHUB_URL, LICENSE_URL, RELEASES_URL } from './site';

export function softwareApplication(lang: 'en' | 'ru', description: string, url: string, image: string) {
  return {
    '@context': 'https://schema.org',
    '@type': 'SoftwareApplication',
    name: 'Tabless',
    description,
    url,
    image,
    applicationCategory: 'BusinessApplication',
    applicationSubCategory: lang === 'ru' ? 'Менеджер задач' : 'Task manager',
    operatingSystem: 'Windows 10, Windows 11, macOS 12+, Linux',
    offers: { '@type': 'Offer', price: '0', priceCurrency: 'USD' },
    license: LICENSE_URL,
    downloadUrl: RELEASES_URL + '/latest',
    codeRepository: GITHUB_URL,
    sameAs: [GITHUB_URL],
    inLanguage: ['en', 'ru'],
    isAccessibleForFree: true,
    keywords: lang === 'ru'
      ? 'офлайн канбан-доска, менеджер задач без интернета, планировщик для разработчиков, Jira, GitHub, Vim'
      : 'offline kanban board, offline task manager, developer planner, local-first, Jira, GitHub, Vim',
  };
}

export function faqPage(items: [string, string][]) {
  return {
    '@context': 'https://schema.org',
    '@type': 'FAQPage',
    mainEntity: items.map(([q, a]) => ({ '@type': 'Question', name: q, acceptedAnswer: { '@type': 'Answer', text: a } })),
  };
}

/** For `<script type="application/ld+json" set:html=…>`: JSON with `<` escaped so no text can close the tag. */
export function ld(data: unknown): string {
  return JSON.stringify(data).replace(/</g, '\\u003c');
}
