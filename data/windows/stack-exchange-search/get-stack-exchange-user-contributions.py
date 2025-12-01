#!/usr/bin/python3

import requests
import time
import datetime
import json
import argparse
from pathlib import Path

API_ROOT = "https://api.stackexchange.com/2.3"

# We get information in time slices of one year because there is a 25 maximum page limit for anonymous API access:
# https://stackapps.com/questions/9343/how-can-i-retrieve-more-than-25-pages-from-the-stack-exchange-api-v2-3
# Maximum page size of 100 * 25 = 2500, so as long as there are no more than 2500 entries in one year for a given activity then this works (if this limit is hit then expect an HTTP 400 error response)
# Lifting this limit requires authenticating to the Stack Exchange API as a registered application, which is a lot of work, so we take this approach instead

def get_account_creation_year(user_id, site, api_key=None):
    print(f"[i] Fetching account creation date for user {user_id} on {site}")
    params = {
        "site": site,
        "pagesize": 1
    }
    if api_key:
        params["key"] = api_key

    url = f"{API_ROOT}/users/{user_id}"
    response = requests.get(url, params=params)
    data = response.json()

    if 'items' not in data or not data['items']:
        raise Exception("Could not retrieve user info.")

    creation_date = data['items'][0]['creation_date']
    creation_year = datetime.datetime.fromtimestamp(creation_date, datetime.UTC).year
    print(f"[✓] Account created in {creation_year}")
    return creation_year

def fetch_user_activity(user_id, site, activity_type, creation_year, api_key=None):
    current_year = datetime.datetime.now(datetime.UTC).year
    all_items = []

    for year in range(current_year, creation_year - 1, -1):
        print(f"[i] Fetching {activity_type} from year {year}")
        fromdate = int(datetime.datetime(year, 1, 1, tzinfo=datetime.UTC).timestamp())
        todate = int(datetime.datetime(year + 1, 1, 1, tzinfo=datetime.UTC).timestamp()) - 1
        page = 1

        while True:
            params = {
                # Chosen Stack Exchange site
                "site": site,
                # Maximum page size allowed by API
                "pagesize": 100,
                # Page number to retrieve
                "page": page,
                # Time slice
                "fromdate": fromdate,
                "todate": todate,
                # Reverse chronological order
                "order": "desc",
                "sort": "creation",
               # Include contents in output, not just metadata
               "filter": "withbody",
            }
            if api_key:
                params["key"] = api_key

            url = f"{API_ROOT}/users/{user_id}/{activity_type}"
            response = requests.get(url, params=params)
            data = response.json()

            if "backoff" in data:
                print(f"[!] Backoff requested. Sleeping for {data['backoff']} seconds.")
                time.sleep(data["backoff"])

            if "items" not in data:
                raise Exception(f"Error fetching data: {data}")

            all_items.extend(data["items"])
            print(f"  └─ Page {page}: Fetched {len(data['items'])} items")

            if not data.get("has_more"):
                break
            page += 1

    return all_items

def main():
    parser = argparse.ArgumentParser(description="Fetch Stack Exchange user activity")
    parser.add_argument("user_id", help="User ID", type=int)
    parser.add_argument("site", help="Stack Exchange site (e.g. stackoverflow, superuser)")
    parser.add_argument("--activity", choices=["questions", "answers", "comments", "all"], default="all")
    parser.add_argument("--output", help="Output JSON file (defaults to <user_id>_<activity>.json)")
    parser.add_argument("--api-key", help="Optional Stack Exchange API key")

    args = parser.parse_args()
    activities = ["questions", "answers", "comments"] if args.activity == "all" else [args.activity]

    creation_year = get_account_creation_year(args.user_id, args.site, args.api_key)
    results = {}

    for activity in activities:
        print(f"[→] Fetching {activity}")
        items = fetch_user_activity(args.user_id, args.site, activity, creation_year, args.api_key)
        results[activity] = items
        print(f"[✓] Fetched {len(items)} {activity}")

    output_file = args.output or f"{args.user_id}_{args.activity}.json"
    with Path(output_file).open("w", encoding="utf-8") as f:
        json.dump(results if args.activity == "all" else results[args.activity], f, indent=2)

    print(f"[✓] Done. Saved to {output_file}")

if __name__ == "__main__":
    main()
