#include "stdio.h"
#include "stdlib.h"
#include <unistd.h>
#include "salsa20_core.h"
#include <string.h>
#include <stdlib.h>
#include <time.h>

unsigned variant = 0;
double result_time = 0;

u_int32_t magicnumber[4] = {23423, 123523, 34566, 27456};

union
{
    u_int8_t *int8arr;
    u_int32_t *int32arr;
} outputTmp;

union
{
    u_int8_t *int8arr;
    u_int32_t *int32arr;
} key;

int error_message(FILE *file, char *error_message)
{
    printf("%s\n", error_message);
    fclose(file);
    return -1;
}
struct Variant
{
    const char *name;
    void (*func)(u_int32_t *, const u_int32_t *);
};

typedef struct Variant Variant;

const Variant variants[] = {
    {"asm", salsa20_core_naive},
    {"simd", salsa20_core},
    {"c", calc_salsa_arr},
    {"intrinsics", calc_salsa_intrinsics}};

static double curtime(void)
{
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec + t.tv_nsec * 1e-9;
}

static void run(void (*func)(u_int32_t *, const u_int32_t *), u_int32_t *output, const u_int32_t *input)
{
    double start = curtime();
    for (size_t i = 0; i < 1000; i++)
    {
        func(output, input);
    }
    double end = curtime();
    result_time += (end - start) /1000 ;
}

static void print(const char *name, double result)
{
    printf("%-4s took %f seconds\n", name, result);
}

u_int32_t *initializeInput(u_int32_t *input, u_int32_t *key, u_int64_t iv)
{
    /*initialize the input matrix*/
    input[0] = magicnumber[0];
    for (int i = 0; i < 4; i++)
    {
        input[1 + i] = key[i];
    }
    input[5] = magicnumber[1];
    /*set nonce*/
    input[7] = iv;
    /*shift 32 bit of 64 bit long nonce to left*/
    iv <<= 32;
    input[6] = iv;
    /*set counter*/
    input[8] = 0L;
    input[9] = 0L;
    input[10] = magicnumber[2];
    for (int i = 4; i < 8; i++)
    {
        input[7 + i] = key[i];
    }
    input[15] = magicnumber[3];
    return input;
}

void run_test(size_t mlen, const u_int8_t msg[mlen], u_int8_t cipher[mlen],
              u_int32_t *key, u_int64_t iv)
{

    u_int32_t *input = malloc(sizeof(u_int32_t) * 16);
    u_int32_t *output = malloc(sizeof(u_int32_t) * 16);

    /*initialize the input matrix*/
    input = initializeInput(input, key, iv);
    /*run the algorithm depending on the variant*/
    for (int k = 0; k < sizeof(variants) / sizeof(variants[0]); k++)
    {
        const Variant *variantStruct = &variants[k];

        //temporary length for decrementing
        size_t tmplen = mlen;

        //index for the offset which incremented with each run
        int j = 0;

        do
        {
            run(variantStruct->func, output, input);
            outputTmp.int32arr = output;
            int offset = 64 * j;

            /*input has the length of 32 * 16 Bit. One run can cover 64 Byte of the message.*/
            for (int i = 0; ((offset + i) < mlen) & (i < 64); i++)
            {
                cipher[offset + i] = outputTmp.int8arr[i] ^ msg[offset + i];
                tmplen--;
            }

            /*increment the counter*/
            u_int32_t tmpCounter2 = input[9];

            input[8]++;

            if (!input[8])
            {
                input[9]++;
                //overflow
                if (tmpCounter2 > input[9])
                {
                    error_message(stderr, "File is too big");
                }
            }
            j++;
        } while ((tmplen != 0) & (tmplen < mlen));
        print(variantStruct->name, result_time);
        result_time = 0;
    }
}

void salsa20_crypt(size_t mlen, const u_int8_t msg[mlen], u_int8_t cipher[mlen],
                   u_int32_t *key, u_int64_t iv)
{
    u_int32_t *input = malloc(sizeof(u_int32_t) * 16);
    u_int32_t *output = malloc(sizeof(u_int32_t) * 16);

    input = initializeInput(input, key, iv);
    /*run the algorithm depending on the variant*/
    const Variant *variantStruct = &variants[variant];
    //index for the offset which incremented with each run
    int j = 0;
    size_t tmplen = mlen;

    do
    {
        run(variantStruct->func, output, input);
        outputTmp.int32arr = output;
        int offset = 64 * j;

        /*input has the length of 32 * 16 Bit. One run can cover 64 Byte of the message.*/
        for (int i = 0; ((offset + i) < mlen) & (i < 64); i++)
        {
            cipher[offset + i] = outputTmp.int8arr[i] ^ msg[offset + i];
            tmplen--;
        }

        /*increment the counter*/
        /*increment the counter*/
        u_int32_t tmpCounter2 = input[9];

        input[8]++;

        if (!input[8])
        {
            input[9]++;
            //overflow
            if (tmpCounter2 > input[9])
            {
                error_message(stderr, "File is too big");
            }
       }

        j++;
    } while ((tmplen != 0) & (tmplen < mlen));

    print(variantStruct->name, result_time);
}

size_t writeData(const char *filename, size_t mlen, u_int8_t cipher[mlen], u_int8_t *key, u_int64_t nonce)
{
    if (*filename == '-')
    {
        printf("Nonce: %ld\n", nonce);

        printf("Key: ");
        for (int i = 0; i < 32; i++)
        {
            printf("%c", key[i]);
        }
        printf("\n");

        printf("Encrypted data: ");
        for (int i = 0; i < mlen; i++)
        {
            printf("0x%x", cipher[i]);
            printf(" ");
        }
        printf("\n");
    }
    else
    {
        FILE *file = fopen(filename, "wb");
        if (file == NULL)
        {
            return error_message(stderr, "File could not be opened");
        }

        for (int i = 0; i < mlen; i++)
        {
            fputc(cipher[i], file);
        }

        printf("Nonce: %ld\n", nonce);
        printf("Key: ");
        for (int i = 0; i < 32; i++)
        {
            putchar(key[i]);
        }
        printf("\nEncrypted data can be found in %s\n", filename);
        fclose(file);
    }
    return 0;
}

/* reads from the file into the given data and returns the length of the file. If the filename is "-"
    it reads from the stdin */
size_t readData(const char *filename, u_int8_t **data)
{
    size_t mlen = 0;

    if (*filename == '-')
    {
        char *tmp = malloc(500);
        printf("Enter the data: ");
        int ch, n = 0;

        while ((ch = getchar()) != '\n' && n < 500)
        {
            tmp[n] = ch;
            n++;
        }

        /*reallocate memory for the data in real size*/
        *data = malloc(n);
        *data = (u_int8_t *)tmp;
        mlen = n;
    }
    else
    {
        /*Read the file in read binary mode*/
        FILE *file = fopen(filename, "rb");
        if (file == NULL)
        {
            return error_message(stderr, "File could not be opened");
        }

        /*Length of the file*/
        size_t length = 0;
        if (fseek(file, 0L, SEEK_END) == 0)
        {
            length = ftell(file);
            rewind(file);
        }
        else
        {
            return error_message(file, "fseek() to end of the file failed");
        }

        /*Length of the file cannot be 0*/
        if (length == 0)
        {
            return error_message(file, "File is empty");
        }

        *data = malloc(length);
	if(data == NULL) {
		return error_message(stderr, "File is too big");
	}
        mlen = fread(*data, 1, length, file);
        if (mlen != length)
        {
            return error_message(stderr, "Error while reading the file");
        }
        fclose(file);
    }
    return mlen;
}
int main(int argc, char **argv)
{
    const char *filename = "-";
    
    //default nonce
    u_int64_t nonce = 255;
    u_int8_t keyTmp[32];
    for(int i = 0; i < 32; i++) {
    	keyTmp[i] = 0;
    }
    
    //default key
    key.int8arr = keyTmp;

    /*flag for the test*/
    int test = 0;

    int opt;
    int valid = 1;
    while (((opt = getopt(argc, argv, "tv:n:k:f:h")) != -1) & (valid == 1))
    {
        switch (opt)
        {
        case 't':
            test = 1;
            break;
        case 'v':
            variant = strtoul(optarg, NULL, 0);
            break;
        case 'n':
            nonce = strtoll(optarg, NULL, 0);
            break;
        case 'k':
        {
            int length = strlen(optarg);
            if (length <= 32)
            {
                for (size_t i = 0; i < length; i++)
                {
                    keyTmp[i] = optarg[i];
                }
                key.int8arr = keyTmp;
            }else{
	    	error_message(stderr, "key is too big");
		valid = 0;
	    }
            break;
        }
        case 'f':
            filename = optarg;
            break;
        case 'h':
        {
            printf("usage: %s [-v variant] [-n nonce] [-k key] [-f infile]\nexample: %s -v 1 -n 249 -k hello -f example.txt\n", argv[0], argv[0]);
            printf("variant types:\n0: asm\n1: simd\n2: c\n3: intrinsics\n");
	    valid = 0;
            break;
        }
        default:
            fprintf(stderr, "usage: %s [-v variant] [-n nonce] [-k key] [-f infile]\n", argv[0]);
            fprintf(stderr, "example: %s -v 1 -n 249 -k hello -f example.txt\n", argv[0]);
	    valid = 0;
            break;
        }
    }

    if(valid == 0) {
    	return EXIT_FAILURE;
    }
    
    if(argc == 1) {
    	fprintf(stderr, "usage: %s [-v variant] [-n nonce] [-k key] [-f infile]\n", argv[0]);
            fprintf(stderr, "example: %s -v 1 -n 249 -k hello -f example.txt\n", argv[0]);
            return EXIT_FAILURE;
    }
    if (optind < argc - 1)
        fprintf(stderr, "%s: ignoring extra arguments\n", argv[0]);

    if (variant >= sizeof(variants) / sizeof(Variant))
    {
        fprintf(stderr, "%s: invalid variant %u\n", argv[0], variant);
        return EXIT_FAILURE;
    }

    /*read the data*/
    u_int8_t *data;
    size_t mlen = readData(filename, &data);
    
    if(mlen == -1) {
    	return EXIT_FAILURE;
    }
    /*allocate memory for the output file*/
    u_int8_t *cipher = malloc(mlen);
    if(cipher == NULL) {
    	error_message(stderr, "File is too big");
    }

    /*allocate memory for the input file*/
    u_int8_t *msg = malloc(mlen);
    if(msg == NULL) {
    	error_message(stderr, "File is too big");
    }
    for (int i = 0; i < mlen; i++)
    {
        msg[i] = data[i];
    }

    /*if test flag is set, execute the test*/
    if (test)
    {
        run_test(mlen, msg, cipher, key.int32arr, nonce);
        writeData(filename, mlen, cipher, key.int8arr, nonce);
        return EXIT_SUCCESS;
    }
    salsa20_crypt(mlen, msg, cipher, key.int32arr, nonce);
    writeData(filename, mlen, cipher, key.int8arr, nonce);
    return 0;
}
