#!/usr/bin/env node

const program = require('commander');

/*-------------------
-    Funtion Loop
--------------------*/
function showhelloloop(repeat) {
    const tiempoInicial = process.hrtime();

    for (let i = 1; i <= repeat; i++) {
        console.log('Hello World:', i);
    }

    const tiempoFinal = process.hrtime(tiempoInicial);
    console.log(`Total time taken by CPU: ${(tiempoFinal[0] + (tiempoFinal[1] / 1000000000)).toFixed(5)}s. Total loops:${repeat}`);
}

/*----------------
-       MAIN
-----------------*/
function main() {
    var repeat = 100;

    // Define Menu Opc.
    program
        .command('./02helloworldloop.js')
        .description('Command Line Interface Hello World')
        .usage('[-h] [-v] [-r REPEAT]')
        .option('-r, --repeat <REPEAT>', 'Repeat execution "Hello World" times', parseInt, 100)
        .version('Hello World Loop 1.0.0', '-v, --version')
        .parse(process.argv);

    const options = program.opts();

    if (options.repeat) {
        repeat = options.repeat
    }

    // Ejec. Funtion Loop
    showhelloloop(repeat);
}

main();
