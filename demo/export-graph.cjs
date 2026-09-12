'use strict';
// Usage: node demo/export-graph.cjs 2048 817 > map.edges
const {makeMap}=require('./map-generator.js');
const n=Number(process.argv[2]??2048),seed=Number(process.argv[3]??817);
const sizes=new Map([[64,[8,8]],[256,[16,16]],[1024,[32,32]],[2048,[64,32]],[4096,[64,64]],[16384,[128,128]]]);
if(!sizes.has(n)||!Number.isInteger(seed)||seed<0||seed>999999){console.error('Use a supported demo size and an integer seed from 0 to 999999.');process.exit(1);}
const [side,rows]=sizes.get(n),g=makeMap(side,seed,rows),lines=[`# Fourfold Voronoi map: seed ${seed}, fingerprint ${g.fingerprint}`,`${g.n} ${g.edges.length/2}`];
for(let i=0;i<g.edges.length;i+=2)lines.push(`${g.edges[i]} ${g.edges[i+1]}`);
process.stdout.write(lines.join('\n')+'\n');
