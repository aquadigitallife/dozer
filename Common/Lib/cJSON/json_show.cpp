#include "stdio.h"
#include "cJSON.h"


void show_json_obj(const cJSON *jobj)
{
	if (jobj == NULL) { printf("jobj is null\r\n"); return; }
	for (const cJSON *item = jobj->child; item != NULL; item = item->next) {
		printf("%s ", item->string);
		switch (item->type) {
			case cJSON_False:
				printf("false\r\n");
				break;
			case cJSON_True:
				printf("true\r\n");
				break;
			case cJSON_Number:
				printf("%.3f\r\n", item->valuedouble);
				break;
			case cJSON_String:
				printf("%s\r\n", item->valuestring);
				break;
			case cJSON_Array:
			{
				const cJSON *sitem;
				printf("array:\r\n");
				cJSON_ArrayForEach(sitem, item)
				{
					for (const cJSON *ch = sitem->child; ch != NULL; ch = ch->next) {
						printf("    %s ", ch->string);
						switch (ch->type) {
							case cJSON_False:
								printf("false");
								break;
							case cJSON_True:
								printf("true");
								break;
							case cJSON_Number:
								printf("%.3f", ch->valuedouble);
								break;
							case cJSON_String:
								printf("%s", ch->valuestring);
								break;
							default:
								printf("???");
						}
					}
					printf("\r\n");
				}
				break;
			}
			default:
				printf("???\r\n");
		}
	}
}
