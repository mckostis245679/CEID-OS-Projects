#!/bin/bash

PASSENGERS_FILE="mypassenger.csv"


# Function to insert data into $PASSENGERS_FILE
insert_data() {
    echo "Insert data:"
    read -p "Enter the filename (or press Enter to input manually): " filename

    if [[ -z "$filename" ]]; then
        echo "Enter passenger data in the format: [code],[fullname],[age],[country],[status],[rescued]:"
        while read -r line; do
            if [[ -z "$line" ]]; then
                break
            fi
            # Validate the format of the input line and replace commas with semicolons
            if [[ "$line" =~ ^[A-Za-z0-9]+,[A-Za-z]+[[:space:]]+[A-Za-z]+,[0-9]+,[^,]+,(Passenger|Crew),(Yes|No)$ ]]; then
                # Replace commas with semicolons and save to the file
                echo "$line" | tr ',' ';' >> "$PASSENGERS_FILE"
            else
                echo "Invalid format: $line"
            fi
        done
    else
        if [[ -f "$filename" ]]; then
            # Convert commas to semicolons before appending to the main file
            cat "$filename" | tr ',' ';' >> "$PASSENGERS_FILE"
        else
            echo "Error: File '$filename' not found."
        fi
    fi
}





# Function to search for a passenger
search_passenger() {
    read -p "Enter the first name or last name of the passenger: " name
    # Use awk to search for the passenger's details with semicolon as the field separator
    results=$(awk -F'[;,]' -v search="$name" '{
        split($2, names, " ");  # Split the fullname into first and last names
        if (tolower(names[1]) == tolower(search) || tolower(names[2]) == tolower(search)) {
            print $0
        }
    }' "$PASSENGERS_FILE")
    
    if [[ -n "$results" ]]; then
        echo "Search results:"
        echo "$results"
    else
        echo "Passenger not found."
    fi
}



# Function to update passenger data
update_passenger() {
    read -p "Enter the code, name, or surname of the passenger: " identifier
    read -p "Enter the field to update (or 'record' for the entire record): " field

    old_record=$(grep -i "$identifier" $PASSENGERS_FILE)
    if [[ -z "$old_record" ]]; then
        echo "Passenger not found."
        return
    fi

    if [[ "$field" == "record" ]]; then
        read -p "Enter the new record: " new_record
        sed -i "/$identifier/c\$new_record" $PASSENGERS_FILE
    else
        read -p "Enter the new value: " new_value
        awk -F, -v OFS="," -v id="$identifier" -v field="$field" -v value="$new_value" \
        '{
            if ($0 ~ id) {
                if (field == "code") $1 = value
                else if (field == "fullname") $2 = value
                else if (field == "age") $3 = value
                else if (field == "country") $4 = value
                else if (field == "status") $5 = value
                else if (field == "rescued") $6 = value
            }
            print
        }' $PASSENGERS_FILE > temp && mv temp $PASSENGERS_FILE
    fi

    echo "Old record: $old_record"
    echo "Updated record: $(grep -i "$identifier" $PASSENGERS_FILE)"
}

# Function to display the file with pagination
display_file() {
    less $PASSENGERS_FILE
}

# Function to generate reports
generate_reports() {
    echo "Generating reports..."

    # Age groups
    awk -F, '{
        if ($3 <= 18) age_groups["0-18"]++
        else if ($3 <= 35) age_groups["19-35"]++
        else if ($3 <= 50) age_groups["36-50"]++
        else age_groups["51+"]++
    } END {
        for (group in age_groups) {
            print group, age_groups[group]
        }
    }' $PASSENGERS_FILE > ages.txt

    # Rescue percentages
    awk -F, '{
        if ($3 <= 18) {
            total["0-18"]++
            if ($6 == "Yes") rescued["0-18"]++
        } else if ($3 <= 35) {
            total["19-35"]++
            if ($6 == "Yes") rescued["19-35"]++
        } else if ($3 <= 50) {
            total["36-50"]++
            if ($6 == "Yes") rescued["36-50"]++
        } else {
            total["51+"]++
            if ($6 == "Yes") rescued["51+"]++
        }
    } END {
        for (group in total) {
            printf "%s %.2f%%\n", group, (rescued[group] / total[group]) * 100
        }
    }' $PASSENGERS_FILE > percentages.txt

    # Average age per category
    awk -F, '{
        if ($5 == "Passenger") {
            sum_passenger += $3; count_passenger++
        } else if ($5 == "Crew") {
            sum_crew += $3; count_crew++
        }
    } END {
        printf "Passenger: %.2f\nCrew: %.2f\n", sum_passenger / count_passenger, sum_crew / count_crew
    }' $PASSENGERS_FILE > avg.txt

    # Rescued passengers
    grep -i "Yes" $PASSENGERS_FILE > rescued.txt

    echo "Reports generated: ages.txt, percentages.txt, avg.txt, rescued.txt"
}

# Main script logic
case "$1" in
    insert)
        insert_data
        ;;
    search)
        search_passenger
        ;;
    update)
        update_passenger
        ;;
    display)
        display_file
        ;;
    reports)
        generate_reports
        ;;
    *)
        echo "Usage: $0 {insert|search|update|display|reports}"
        ;;
esac
