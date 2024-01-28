// This code was written by Matthew Mahan for Cyberstorm.
// Other team members include Trey Burkhalter, Christian Brunning, and Aaron Earp. 

// Source for libcurl code can be found at: https://www.youtube.com/playlist?list=PLA1FTfKBAEX6p-lfk1l_Q2zh2E5wd-cup
// Source for sha256 code can be found at: https://stackoverflow.com/questions/2262386/generate-sha256-with-openssl-and-c

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <curl/curl.h>
#include <openssl/sha.h>
#include <stdbool.h>

#define ANSI_FOREGROUND_GREEN "\e[0;32m"
#define ANSI_FOREGROUND_RED "\e[0;31m"
#define ANSI_FOREGROUND_WHITE "\e[0;37m"

#define SKIP_CONNECTION_TEST true

// function pointer for consistent question format
typedef bool (*questionCallback)(char*);

struct MemoryStruct {
  char *memory;
  size_t size;
};

typedef struct Question {
	char* message;
	questionCallback callback;
	struct Question* nextQuestion;
} tQuestion;
 
// callback for libcurl memory chunks 
static size_t
WriteMemoryCallback(void *contents, size_t size, size_t nmemb, 
                    void *userp)
{
  size_t realsize = size * nmemb;
  struct MemoryStruct *mem = (struct MemoryStruct *)userp;
  
  char *ptr = realloc(mem->memory, mem->size + realsize + 1);
  if(!ptr) 
  {
    printf("not enough memory (realloc returned NULL)\n");
    return CURL_WRITEFUNC_ERROR;
  }
 
  mem->memory = ptr;
  memcpy(&(mem->memory[mem->size]), contents, realsize);
  mem->size += realsize;
  mem->memory[mem->size] = 0;
 
  return realsize;
}

int send_user_account(char* username, char* hash, char* server_ip, char* server_port) {

	printf("Sending user account creation request...\n");

	CURL *curl;
	CURLcode result;
	char url[2048];

	struct MemoryStruct chunk;
	chunk.memory = malloc(1);
	chunk.size = 0;

	curl = curl_easy_init();

	if (curl == NULL) {
		fprintf(stderr, "HTTP request failed/n");
		return -1;
	}

	char* url_encoded_username = curl_easy_escape(curl, username, 0);
	char* url_encoded_hash = curl_easy_escape(curl, hash, 0);

	sprintf(url, "http://%s:%s?username=%s&hash=%s", server_ip, server_port, url_encoded_username, url_encoded_hash);

	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);

	result = curl_easy_perform(curl);

	if (result != CURLE_OK) {
		fprintf(stderr, "Error: %s\n", curl_easy_strerror(result));
		return -1;
	}

	printf("Reply from remote host: %s\n", chunk.memory);
	
	curl_free(url_encoded_username);
	curl_free(url_encoded_hash);
	curl_easy_cleanup(curl);

	free(chunk.memory);

	return 0; 
}

int test_connection(char* server_ip, char* server_port) {

	printf("Testing connection...\n");

	CURL *curl;
	CURLcode result;
	char url[2048];

	struct MemoryStruct chunk;
	chunk.memory = malloc(1);
	chunk.size = 0;

	curl = curl_easy_init();

	if (curl == NULL) {
		fprintf(stderr, "HTTP request failed/n");
		return -1;
	}

	sprintf(url, "http://%s:%s", server_ip, server_port);

	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);

	result = curl_easy_perform(curl);

	if (result != CURLE_OK) {
		fprintf(stderr, "Error: %s\n", curl_easy_strerror(result));
		return -1;
	}

	printf("Reply from remote host: %s\n", chunk.memory);
	curl_easy_cleanup(curl);
	free(chunk.memory);

	return 0; 
}

// hash function sha256 which will also be used on the server side
void sha256_hash_string (unsigned char hash[SHA256_DIGEST_LENGTH], char outputBuffer[65])
{
    int i = 0;

    for(i = 0; i < SHA256_DIGEST_LENGTH; i++)
    {
        sprintf(outputBuffer + (i * 2), "%02x", hash[i]);
    }

    outputBuffer[64] = 0;
}

// only accepts a single word
// whitespace will not be included in response 
void get_input_with_message(char* message, char* userinput) {

	if (message) {
		printf("%s\n>>> ", message);
	}
	scanf("%s", userinput); 
	printf("\n");

	return;

}

void color_string(char* message, char* color, char* colorstr) {
	strcat(colorstr, color);
	strcat(colorstr, message);
	strcat(colorstr, ANSI_FOREGROUND_WHITE);
}

// This function prints the list but also is responsible for checking correctness 
bool print_question_list(tQuestion* q, int stop, char* password) {

	bool allCorrect = true;
	// printf("%d\n", stop);

	for (int i = 0; i < stop; i++) {

		char colorstr[1024] = "";

		// set the color according to if the question is fulfilled 
		if (q->callback(password)) {
			color_string(q->message, ANSI_FOREGROUND_GREEN, colorstr);
		} else {
			color_string(q->message, ANSI_FOREGROUND_RED, colorstr);
			allCorrect = false;
		}

		printf("%s\n", colorstr);

		if (!allCorrect) {
			return allCorrect;
		}

		if (q->nextQuestion == NULL) {
			return allCorrect;
		}

		q = q->nextQuestion;
	}

}

// manages server configuration on program start
// asks the user where the server is 
void opening_config(char* server_ip, char* server_port) {

	get_input_with_message("What is the ip of the server?", server_ip);
	get_input_with_message("What is the port of the server?", server_port);

}

int get_total_questions(tQuestion* q) {

	int total = 0;

	while (q != NULL) {
		total++;
		q = q->nextQuestion;
	}	

	return total;

}

// question functions 

bool example_function(char* message) {
	printf("%s\n", message);
	return true;
}

// "Your password must contain a number."
bool question1(char* message) {
	return true;
}

// "Your password must contain a capital letter."
bool question2(char* message) {
	return true;
}


// "Your password must contain a special character from this list: !@#$^&*()+=~"
bool question3(char* message) {
	return true;
}

// "The length of your password must be a highly composite number."
bool question4(char* message) {
	return true;
}

// "Your password must contain one of our sponsors: Pepsi Walmart Lowes LEGO Autozone Build-A-Bear"
bool question5(char* message) {
	return false;
}

// "Your password must contain one word of university spirit: Anky Timo Bulldogs LATech Cyberstorm"
bool question6(char* message) {
	return false;
}


// "Your password must contain a roman numeral."
bool question7(char* message) {
	return false;
}

// "The digits in your password must sum to 18 or more."
bool question8(char* message) {
	return false;
}

// "Our sponsors list has been updated. Your password must NOT contain an old sponsor: Walmart Autozone Pepsi"
bool question9(char* message) {
	return false;
}

// "Your password must contain your birthday in MMMDDYYYY format. Example: Jan011970"
bool question10(char* message) {
	return false;
}

// "Your password must contain your star sign."
bool question11(char* message) {
	return false;
}


// Entry point 
int main (void) {

	char server_ip[64];
	char server_port[8];

	printf("\n\nWelcome to the Secure Account Generator!\n\n");

	if (!SKIP_CONNECTION_TEST) {
		opening_config(server_ip, server_port);
		if (test_connection(server_ip, server_port)) {
			printf("\nConnection failed! Aborting...\n");
			exit(-1);
		}
	}

	// winning variables
	char username[64] = "";
	char password[2048] = "";

	// question struct instances
	// MUST be defined in reverse order so that list can be made
	// struct Question test = { "Hey this is a question!", example_function };
	tQuestion q11 = { "Your password must contain your star sign.", question11, NULL };
	tQuestion q10 = { "Your password must contain your birthday in MMMDDYYYY format. Example: Jan011970", question10, &q11 };
	tQuestion q9 = { "Our sponsors list has been updated. Your password must NOT contain an old sponsor: Walmart Autozone Pepsi", question9, &q10 };
	tQuestion q8 = { "The digits in your password must sum to 18 or more.", question8, &q9 };
	tQuestion q7 = { "Your password must contain a roman numeral.", question7, &q8 };
	tQuestion q6 = { "Your password must contain one word of university spirit: Anky Timo Bulldogs LATech Cyberstorm", question6, &q7 };
	tQuestion q5 = { "Your password must contain one of our sponsors: Pepsi Walmart Lowes LEGO Autozone Build-A-Bear", question5, &q6 };
	tQuestion q4 = { "The length of your password must be a highly composite number.", question4, &q5 }; 
	tQuestion q3 = { "Your password must contain a special character from this list: !@#$^&*()+=~", question3, &q4 };
	tQuestion q2 = { "Your password must contain a capital letter.", question2, &q3 };
	tQuestion q1 = { "Your password must contain a number.", question1, &q2 };
	
	// main game loop
	tQuestion* first = &q1;
	int questions_found = 0;
	int total_questions = get_total_questions(first);
	bool allCorrect = false;
	bool prevAllCorrect = false;
	while (true) {

		// check previous attempt and print all the questions
		allCorrect = print_question_list(first, questions_found, password);

		// prompt for user input 
		get_input_with_message("Please enter a new password.", password);

		prevAllCorrect = allCorrect;

		// check if no more questions (Win state, sbreak loop)
		if (allCorrect && questions_found == total_questions) {
			break;
		}

		// increment found questions 
		questions_found++;

	}

	// win state
	char hashed[64];
	sha256_hash_string(password, hashed);
	send_user_account(username, hashed, server_ip, server_port);

	return 0;
}