
PASSENGERS_FILE="passenger_data.csv"


insert_data() {
    echo "Enter the file path (or leave blank to input manually):"
    read filepath

    if [[ -z $file ]]; then
        echo "Enter passenger data in the format:"
        echo "[code],[fullname],[age],[country],[status (Passenger/Crew)],[rescued (Yes/No)]"
        echo "Type 'END' when you are done."

        while read -r line; do
            if [[ -z "$line" ]]; then
                break
            fi

            if [[ "$line" =~ ^[0-9]+,[A-Za-z]+[[:space:]]+[A-Za-z]+,[0-9]+,[A-Za-z[:space:]]+,(Passenger|Crew),(Yes|No)$ ]]; then # Checking validation of format
                echo "$line" | tr ',' ';' >> "$PASSENGERS_FILE"  # Replace , with ; and append to csv

			elif [ "$line" = "END" ]; then break
            
            else
                echo "Invalid format: $line"
            fi
        done

    elif [[ -f $file ]]; then cp "$file" "$PASSENGERS_FILE"

    else
        echo "File Does not Exist..."
    fi
}

search_passenger() {
    read -p "Enter the first name or last name of the passenger: " name
    
    if [[ ! -f "$PASSENGERS_FILE" ]]; then
        echo "Passenger file does not exist."
        return 1
    fi

    results=$(awk -F'[;,]' -v search="$name" '
    {
        split($2, names, " ");

        # kanonikopoisi ton periptosewn gia na dexete kai full name
        search = tolower(search)
        first = tolower(names[1])
        last = tolower(names[2])
        full = tolower($2)

        if (first == search || last == search || full == search) {
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

update_passenger() {
    read -p "Enter the code, name, or surname of the passenger: " identifier
    read -p "Enter the field to update (or 'record' for the entire record): " field

    old_record=$(grep -i "$identifier" "$PASSENGERS_FILE")
    if [[ -z "$old_record" ]]; then
        echo "Passenger not found."
        return
    fi

    if [[ "$field" == "record" ]]; then
        read -p "Enter the new record: " new_record
        sed -i "/$identifier/c\$new_record" "$PASSENGERS_FILE"
    else
        read -p "Enter the new value: " new_value
        awk -F';' -v OFS=';' -v id="$identifier" -v field="$field" -v value="$new_value" '
        {
            if ($0 ~ id) {
                if (field == "code") $1 = value
                else if (field == "fullname") $2 = value
                else if (field == "age") $3 = value
                else if (field == "country") $4 = value
                else if (field == "status") $5 = value
                else if (field == "rescued") $6 = value
            }
            print
        }' "$PASSENGERS_FILE" > temp && mv temp "$PASSENGERS_FILE"
    fi

    echo "Old record: $old_record"
    echo ""
    echo "Updated record: $(grep -i "$identifier" "$PASSENGERS_FILE")"
}



# Function to display the file with pagination
display_file() {
    less $PASSENGERS_FILE
}



generate_reports(){
    input="$1"

    if [ -z "$input" ]; then
        input="./passenger_data.csv"
    elif [ ! -f "$input" ]; then
        echo "File does not exist."
        return
    fi

    # gia na ginontai rewrite kathe run
    > "ages.txt"
    > "percentages.txt"
    > "avg.txt"
    > "rescued.txt"

    echo "Generating the age groups"
    awk -F'[;,]' '{
        age = $3;

        if (age >= 0 && age <= 18) {
            group = "0-18";
        } else if (age >= 19 && age <= 35) {
            group = "19-35";
        } else if (age >= 36 && age <= 50) {
            group = "36-50";
        } else if (age >= 51) {
            group = "51+";
        }

        ages[group]++;
    } END {
        for (group in ages) {
            print group ":" ages[group] " passengers";
        }
    }' "$input" > ages.txt

    echo "Generating rescued percent"
awk -F'[;,]' '{
    age = $3;
    if (age >= 0 && age <= 18) {
        group = "0-18";
    } else if (age >= 19 && age <= 35) {
        group = "19-35";
    } else if (age >= 36 && age <= 50) {
        group = "36-50";
    } else if (age >= 51) {
        group = "51+";
    }


    if ( $6 ~ /^[[:space:]]*(Yes|yes)[[:space:]]*$/ || $6 ~ /^[[:space:]]*yes[[:space:]]*$/ ) {
        rescued[group]++;
    }
    total[group]++;
} END {
    for (group in total) {
        if (total[group] > 0) {
            rpercentage = int((rescued[group] / total[group]) * 100);
            print group ":" rescued[group] " / " total[group] " rescued percentage: " rpercentage "%";
        } else {
            print group ": No data available for this group";
        }
    }
}' "$input" > percentages.txt


echo "Generating average age per passenger category"
  awk -F'[;,]' 'NR > 1 { 
    age = $3;
    category = $5;

   
    if (age ~ /^[0-9]+([.][0-9]+)?$/) {
        total[category] += age;
        count[category]++;
    }
  } END {
    for (category in total) {
        if (count[category] > 0) {
            average_age = total[category] / count[category];
            print category ": Average age is: " average_age;
        }
    }
  }' "$input" > avg.txt


echo "Generating file for rescued passengers"
  awk -F'[;,]' '{
   
    if ($6 ~ /^[[:space:]]*(Yes|yes)[[:space:]]*$/) {
        print $0
    }
  }' "$input" > rescued.txt




    echo "Reports generated for:"
    echo "Age Groups in ages.txt"
    echo " Rescue Percentages in  percentages.txt"
    echo "Average Age per Category in avg.txt"
    echo "Rescued Passengers in rescued.txt"
}



# generate_reports() {
#     # Εύρεση ηλικιακών ομάδων
#     awk -F';' '{
#         age=$3;
#         if (age <= 18) print $0 > "ages_0_18.txt";
#         else if (age <= 35) print $0 > "ages_19_35.txt";
#         else if (age <= 50) print $0 > "ages_36_50.txt";
#         else print $0 > "ages_51_plus.txt";
#     }' "$PASSENGERS_FILE"

#     # Ποσοστά διάσωσης ανά ηλικιακή ομάδα
#     awk -F';' '{
#         age=$3; rescued=$6;
#         group = (age <= 18) ? "0-18" : (age <= 35) ? "19-35" : (age <= 50) ? "36-50" : "51+";
#         if (rescued == "Yes") rescued_count[group]++;
#         total_count[group]++;
#     } END {
#         for (group in total_count) {
#             printf "%s: %.2f%%\n", group, (rescued_count[group]/total_count[group])*100;
#         }
#     }' "$PASSENGERS_FILE" > percentages.txt

#     # Μέση ηλικία ανά κατηγορία
#     awk -F';' '{
#         if ($5 == "Passenger") {sum_passengers+=$3; count_passengers++}
#         else if ($5 == "Crew") {sum_crew+=$3; count_crew++}
#     } END {
#         printf "Passenger: %.2f\nCrew: %.2f\n", sum_passengers/count_passengers, sum_crew/count_crew;
#     }' "$PASSENGERS_FILE" > avg.txt

#     # Δημιουργία αρχείου για διασωθέντες
#     grep -i "Yes" "$PASSENGERS_FILE" > rescued.txt

#     echo "Reports Generated: ages.txt, percentages.txt, avg.txt, rescued.txt"
# }




#insert_data;
#update_passenger;
#display_file;
generate_reports;