# ADR 0006: Standardize module ownership for namespace, translation-unit, and header modes

- Status: Accepted

## Context

ArchScope reports metrics per module, but the project supports three distinct
module kinds. The ownership rule must be deterministic so type counts and
dependency edges land in the same module on every run.

## Decision

Define module ownership as follows:

- Namespace mode: owner is the fully qualified namespace containing the type
  definition. Global scope is `<global>`. Anonymous namespaces are emitted as
  `<anonymous>` within their parent scope.
- Translation-unit mode: owner is the normalized source path recorded for the
  translation unit in the compilation database.
- Header mode: owner is the normalized spelling path of the file where the
  type definition appears. If the definition is in a `.cpp`, that source file
  is still the owner.

System-header declarations are excluded from ownership in all modes.

## Alternatives considered

- Assign every symbol to the translation unit that happened to parse it:
  rejected because header-defined types would be double-counted across multiple
  translation units.
- Collapse anonymous and global namespaces into a single bucket: rejected
  because it destroys useful ownership information and complicates filtering.
- Treat only headers as valid header-mode owners: rejected because some types
  are intentionally defined in source files and still need a deterministic
  owner.

## Consequences

- Module identifiers are stable and predictable across runs.
- Header mode can represent both header-defined and source-defined types.
- Filtering behavior can remain module-kind-specific without changing analysis.
