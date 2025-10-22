import { crc32, findSubClasses, makeCRCTable, parseElements, sortEntities, walkParents } from "./gen_functional_types_helpers";
import { Entity } from "./gen_functional_types_interfaces";
import schemaAliases from "./schema_aliases";

const fs = require("fs");

console.log("Starting...");

var files = fs.readdirSync("./");
var expFiles = files.filter((f: string) => f.endsWith(".exp"));

let crcTable = makeCRCTable();

let cSchema: Array<string> = [];
cSchema.push(`// This is a generated file, please see: gen_functional_c_types.js`);
cSchema.push(``);
cSchema.push(`#ifndef IFC_SCHEMA_H`);
cSchema.push(`#define IFC_SCHEMA_H`);
cSchema.push(``);
cSchema.push(`#include <stdint.h>`);
cSchema.push(`#include <stddef.h>`);
cSchema.push(``);
cSchema.push(`#ifdef __cplusplus`);
cSchema.push(`extern "C" {`);
cSchema.push(`#endif`);
cSchema.push(``);

let completeifcElementList = new Set<string>();

let completeEntityList = new Set<string>();
completeEntityList.add("FILE_SCHEMA");
completeEntityList.add("FILE_NAME");
completeEntityList.add("FILE_DESCRIPTION");

let typeList = new Set<string>();
let classList = new Set<string>();

for (var i = 0; i < expFiles.length; i++) {
    let schemaData = fs.readFileSync("./" + expFiles[i]).toString();
    var schemaName = expFiles[i].replace(".exp", "").replace(".", "_");

    let parsed = parseElements(schemaData);
    let entities: Array<Entity> = sortEntities(parsed.entities);
    let types = parsed.types;

    entities.forEach((e) => walkParents(e, entities));
    //now work out the children
    entities = findSubClasses(entities);

    for (var x = 0; x < entities.length; x++) {
        completeEntityList.add(entities[x].name);
        if (entities[x].isIfcProduct) completeifcElementList.add(entities[x].name);
    }

    for (const type of types) {
        if (type.isList) {
            // let typeNum = type.typeNum;
            classList.add(`typedef struct { size_t number; void* value; } ${schemaName}_${type.name};`);
            // tsSchema.push(`export class ${type.name} { type: number=${typeNum}; constructor(public value: Array<${type.typeName}>) {} };`);
            typeList.add(type.name);
        } else if (type.isSelect) {
            classList.add(`typedef struct { void* data; } ${schemaName}_${type.name};`);
        } else if (type.isEnum) {
            //          else if (type.isEnum) {
            //   tsSchema.push(`export class ${type.name} {` + type.values.map((v) => `static ${v} : any =  { type:3, value:'${v}'}; `).join('') + '}');
            // }
            classList.add(`typedef struct { size_t type; void* data; } ${schemaName}_${type.name};`);
        } else {
            // let typeName = type.typeName;
            // let typeNum = type.typeNum;
            // if (type.typeName.search('Ifc') != -1) {
            //     let rawType = types.find(x => x.name == type.typeName);
            //     typeName = rawType!.typeName;
            //     typeNum = rawType!.typeNum;
            // }
            typeList.add(type.name);
        }
    }
}

new Set([...completeEntityList, ...typeList]).forEach(entity => {
    let name = entity.toUpperCase();
    let code = crc32(name, crcTable);
    cSchema.push(`#define ${name} ${code}`);
});

cSchema.push(``);
cSchema.push(`typedef enum {`);
cSchema.push(`  LOGICAL_FALSE = 0,`);
cSchema.push(`  LOGICAL_TRUE = 1,`);
cSchema.push(`  LOGICAL_UNKNOWN = 2`);
cSchema.push(`} logical;`);
cSchema.push(``);
cSchema.push(`typedef enum {`);
let cSchemasValues: Array<string> = [];
for (var i = 0; i < expFiles.length; i++) {
    const schemaName = expFiles[i].replace(".exp", "");
    cSchema.push(`\t${schemaName.replace(".", "_")} = ${i},`);
    cSchemasValues.push(`case ${schemaName.replace(".", "_")}: return "${schemaName}";`);
}
cSchema.push(`} Schemas;`);
cSchema.push(``);
cSchema.push(`static inline const char* schemas_to_string(Schemas s) {`);
cSchema.push(`  switch (s) {`);
cSchema.push(`    ${cSchemasValues.join("\n    ")}`);
cSchema.push(`    default: return "Unknown";`);
cSchema.push(`  }`);
cSchema.push(`}`);
cSchema.push(``);
cSchema.push(`typedef struct { size_t off, len; } Slice;`);
cSchema.push(``);

var expSchemas = expFiles.map((f: string) => {
    var schemaNameClean = f.replace(".exp", "").replace(".", "_");
    return [schemaNameClean, ...schemaAliases.filter((sa) => sa.alias == schemaNameClean).map((sa) => sa.schemaName)];
});

// Emit DATA as flattened array of string literals (one entry per row cell)
const DATA: string[] = [];
const INDEX: { off: number; len: number }[] = [];
for (let r = 0; r < expSchemas.length; r++) {
    const row = expSchemas[r];
    const off = DATA.length;
    for (let j = 0; j < row.length; j++) {
        DATA.push(row[j]);
    }
    INDEX.push({ off, len: row.length });
}
// Emit C arrays: flattened string data and index slices. Consumers can read strings directly.
cSchema.push(`static const char* SCHEMA_NAME_DATA[] = { ${DATA.map(n => `"${n.replace(/"/g, '\\"')}"`).join(", ")} };`);
cSchema.push(`static const Slice SCHEMA_NAME_INDEX[] = { ${INDEX.map(i => `{${i.off},${i.len}}`).join(", ")} };`);
cSchema.push(`static const size_t SCHEMA_NAME_ROWS = sizeof(SCHEMA_NAME_INDEX) / sizeof(SCHEMA_NAME_INDEX[0]);`);
cSchema.push(`static inline const char* get_schema_name(size_t r, size_t c) { return SCHEMA_NAME_DATA[ SCHEMA_NAME_INDEX[r].off + c ]; }`);
cSchema.push(``);

for (let classDef of classList) {
    cSchema.push(classDef);
}
cSchema.push(``);

cSchema.push(`#ifdef __cplusplus`);
cSchema.push(`} /* extern "C" */`);
cSchema.push(`#endif`);
cSchema.push(``);
cSchema.push(`#endif /* IFC_SCHEMA_H */`);
cSchema.push(``);

fs.writeFileSync("../c/ifc_schema.h", cSchema.join("\n"));

console.log(`...Done!`);
