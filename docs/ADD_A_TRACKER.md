# Add your tracker

Tabless reads 13 trackers. If yours is missing and it has a REST API, adding it is usually one function of
about 40 lines and a test. No networking code and no UI: the Settings card, the keychain, paging, retries,
the sync and the board all come from the descriptor.

This page walks through it on Redmine, the smallest provider in the catalog. Open trackers people asked
for are the [`tracker` issues](https://github.com/sectapunterx/tabless/issues?q=is%3Aissue+is%3Aopen+label%3Atracker);
say on one that you're taking it.

## Will it fit?

A tracker fits the generic provider when:

- one **GET** returns the issues assigned to the signed-in user (or the issues of a project you name);
- the request is authorised by **one header** carrying a token (`Authorization: Bearer …`, `PRIVATE-TOKEN: …`,
  `X-Redmine-API-Key: …`);
- the response is JSON, an array of issue objects either at the root or under one key;
- the next page is in a `Link` header, in a field of the body, or reached by stepping an offset.

GraphQL-only APIs (Linear), list-then-fetch APIs (Azure DevOps runs a WIQL query, then fetches the work items)
and token-in-the-query auth (Trello) don't fit. They need a bespoke provider like `JiraProvider` or
`TrelloProvider`. Open an issue first and we'll work out the shape together.

## 1. Read the API

Before writing code, find four things in the tracker's docs and try them with `curl` and your own token:

| What | Redmine's answer |
|---|---|
| "My open issues" endpoint | `GET /issues.json?assigned_to_id=me&status_id=open&limit=100` |
| Auth header | `X-Redmine-API-Key: <key>` |
| Where the array is | `{"issues": [ … ]}` |
| Next page | none in the response: step `offset` until a page comes back short |

Save one real response, trimmed to two or three issues with the private bits replaced. It becomes your test.

## 2. Write the descriptor

In [`src/integrations/ProviderRegistry.cpp`](../src/integrations/ProviderRegistry.cpp), next to `redmine()`:

```cpp
ProviderDescriptor redmine() {
  ProviderDescriptor d;
  d.id = QStringLiteral("redmine");                 // stable: it keys settings and secrets
  d.displayName = QStringLiteral("Redmine");
  d.color = QStringLiteral("#b32024");              // the card swatch
  d.icon = QStringLiteral("R");                     // shown when there is no logo
  d.descKey = QStringLiteral("settings.int.redmine.desc");
  d.uiFields = {plain(QStringLiteral("host"), QStringLiteral("Host"), QStringLiteral("https://redmine.example.com")),
                secret(QStringLiteral("token"), QStringLiteral("API key"))};
  d.requiredKeys = {QStringLiteral("host"), QStringLiteral("token")};
  d.secretKeys = {QStringLiteral("token")};         // keychain, never state.json
  d.baseUrlTemplate = QStringLiteral("{host}");     // or a literal URL for a cloud-only tracker
  d.auth.kind = AuthKind::CustomHeader;
  d.auth.headerName = "X-Redmine-API-Key";
  d.auth.extraHeaders = {{"Accept", "application/json"}};
  d.listPathTemplate = QStringLiteral("/issues.json?assigned_to_id=me&status_id=open&limit=100");
  d.paging.style = PageStyle::Offset;               // keep pageSize equal to the limit above
  d.paging.offsetParam = QStringLiteral("offset");
  d.paging.pageSize = 100;
  d.fields.arrayPointer = QStringLiteral("issues");
  d.fields.id = QStringLiteral("id");
  d.fields.title = QStringLiteral("subject");
  d.fields.body = QStringLiteral("description");
  d.fields.status = QStringLiteral("status.name");  // dot-paths into one issue object
  d.fields.priority = QStringLiteral("priority.name");
  d.fields.urlTemplate = QStringLiteral("{baseUrl}/issues/{id}");
  d.fields.updatedAt = QStringLiteral("updated_on");
  d.fields.assignee = QStringLiteral("assigned_to.name");
  d.fields.dueAt = QStringLiteral("due_date");
  d.fields.project = QStringLiteral("project.name");
  return d;  // pull-only
}
```

Every field and option is documented where it is declared, in
[`ProviderDescriptor.h`](../src/integrations/ProviderDescriptor.h). The ones you'll reach for most:

- `FieldMap` paths: `a.b` goes into objects, `assignees.0.login` indexes an array, `a|b` takes the first that
  isn't empty. A tracker with a done flag instead of a status string uses `boolStatusField` (Todoist, Asana).
- `PageRecipe`: `LinkHeader` (GitHub, GitLab, Gitea), `BodyNext` (Bitbucket, Todoist, Asana), `Offset` (Redmine).
- `selfListPathTemplate` + `scopeKey`: an "assigned to me" endpoint that needs no project, so the card
  works with a token alone.

Then add it to `providerCatalog()` **at the end** of the list. The order is load-bearing, and a test pins
GitHub at 0.

## 3. Name it in both languages

Add the one-line description to [`qml/I18n.qml`](../qml/I18n.qml) in the English and the Russian table:

```js
"settings.int.redmine.desc": "Pull issues assigned to you",
"settings.int.redmine.desc": "Задачи, назначенные на вас",
```

A logo is optional. Without one the card shows `icon`. With one, add a monochrome SVG to
`design/brand-export/icons/`, list it in `CMakeLists.txt` next to the others, and map it in
`providerLogos` in `qml/SettingsView.qml`.

## 4. Test the parse

In [`tests/test_rest_providers.cpp`](../tests/test_rest_providers.cpp), feed the response you saved through
your descriptor and check what comes out:

```cpp
TEST(FieldMapParse, RedmineTrackerProjectAndVersion) {
  const QByteArray json = R"({"issues":[{"id":42,"subject":"Do thing","status":{"name":"In Progress"}, … }]})";
  const QVector<ExternalTask> tasks = parseVia(QStringLiteral("redmine"), json, QStringLiteral("https://r.example.com"));
  ASSERT_EQ(tasks.size(), 1);
  EXPECT_EQ(tasks[0].externalId, QStringLiteral("42"));
  EXPECT_EQ(tasks[0].status, QStringLiteral("In Progress"));
  EXPECT_EQ(tasks[0].url, QStringLiteral("https://r.example.com/issues/42"));
}
```

Build and run `ctest --test-dir build/tests -R rest_providers`.

## 5. Try it against a real account

Run the app on a throwaway data folder, so your own profile is never touched:

```sh
tabless --data-dir /tmp/tabless-try
```

Connect the tracker in Settings → Integrations. Under `--data-dir` the token goes to
`/tmp/tabless-try/secrets.json`, not the keychain. Check that your issues arrive, that their statuses land in
sensible columns, and that a second sync changes nothing. Statuses are matched by name (`open`, `in progress`,
`in review`, `done`, …, see [`StatusMap.cpp`](../src/integrations/StatusMap.cpp)). If yours uses its own
words, say so in the PR.

## 6. Reading only

A new tracker is read-only. Writing a status back (`pushPathTemplate`) is a second step, and any write sits
behind its own per-tracker switch that is off by default. That is [the one rule](INTEGRATIONS.md#the-rule-every-external-write-is-opt-in)
of integrations. A first PR that only reads is the one we can merge quickly.

## 7. Open the PR

Branch off `feature` and open the PR against `feature` ([how the branches work](../CONTRIBUTING.md#workflow)).
In the PR, say:

- which tracker and which API version, with a link to its docs;
- what you tested against: cloud, self-hosted, which version;
- anything the descriptor couldn't express.

Also add the tracker to the list in [`INTEGRATIONS.md`](INTEGRATIONS.md) and in the README. We'll update
the "13 trackers" count on the site and the store listings when it ships.
