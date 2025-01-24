#!/bin/bash

FILE="passenger_data.csv" #GIA DIEUKOLINSH VALTE TO PATH TOU CSV SAS

insert_data() {
    echo "Insert data:"
    read -p "Enter the filename (or press Enter to input manually): " filename

    if [[ -z "$filename" ]]; then
        echo "Enter passenger data in the format: [code],[fullname],[age],[country],[status (Passenger or Crew)],[rescued (Yes or No)]:"
        while read -r line; do
            if [[ -z "$line" ]]; then
                break
            fi

            if [[ "$line" =~ ^[A-Za-z0-9]+,[A-Za-z]+[[:space:]]+[A-Za-z]+,[0-9]+,[^,]+,(Passenger|Crew),(Yes|No)$ ]]; then
                echo "$line" | tr ',' ';' >> "$FILE"
            else
                echo "Invalid format: $line"
            fi
        done
    else
        if [[ -f "$filename" ]]; then
          echo "Enter passenger data in the format: [code],[fullname],[age],[country],[status (Passenger or Crew)],[rescued (Yes or No)]:"
        while read -r line; do
            if [[ -z "$line" ]]; then
                break
            fi

            if [[ "$line" =~ ^[A-Za-z0-9]+,[A-Za-z]+[[:space:]]+[A-Za-z]+,[0-9]+,[^,]+,(Passenger|Crew),(Yes|No)$ ]]; then
                echo "$line" | tr ',' ';' >> "$filename"
            else
                echo "Invalid format: $line"
            fi
        done
        else
            echo "Error: File '$filename' not found."
        fi
    fi
}

search_passenger() {
    read -p "Enter the first name or last name of the passenger: " name

    results=$(awk -F'[;,]' -v search="$name" '{
        split($2, names, " "); 
        if (tolower(names[1]) == tolower(search) || tolower(names[2]) == tolower(search)) {
            print $0
        }
    }' $FILE)

    if [[ -n "$results" ]]; then
        echo "Search results:"
        echo "$results"
    else
        echo "Passenger not found."
    fi
}


update_passenger() {
    person_to_search="$1"
    fieldnvalue="${@:2}"  #ta pairnei ola meta to :

    if [ -z "$filepath" ]; then
        filepath="./passenger_data.csv"
    elif [ ! -f "$filepath" ]; then
        echo "$filepath doesn't exist"
        return
    fi

  
    match=$(grep -i "$person_to_search" "$filepath")

    if [ -z "$match" ]; then
        echo "No match found for '$person_to_search'."
        return
    else
        echo "Matches:"
        echo "$match"
    fi

  
    IFS=":" read -r field value <<< "$fieldnvalue"

    if [ -z "$field" ] || [ -z "$value" ]; then
        echo "Give correct input format"
        return
    fi

    if [ "$field" == "record" ]; then
        if [[ "$value" =~ ^[0-9]+,[A-Za-z]+[[:space:]]+[A-Za-z]+,[0-9]+,[^,]+,(Passenger|Crew),(Yes|No)$ ]]; then
            
            value=$(echo "$value" | tr ',' ';')
            
           
            sed -i "s|$match|$value|" "$filepath"
             echo "New record: $value"
             echo "Old record: $match"
            echo "Full record update successful"
        else
            echo "Invalid input format"
        fi
    else
       
        IFS=";" read -r code fullname age country status rescued <<< "$match"

        
        case "$field" in
            code) code="$value" ;;
            fullname) fullname="$value" ;;
            age) age="$value" ;;
            country) country="$value" ;;
            status) status="$value" ;;
            rescued) rescued="$value" ;;
            *)
                echo "Invalid field. Please use a valid field name."
                return
                ;;
        esac

        
        new_record="$code;$fullname;$age;$country;$status;$rescued"

        
        sed -i "s|$match|$new_record|" "$filepath"
        echo "New record: $new_record"
        echo "Old record: $match"
        echo "Field update successful"
    fi
}



display_file() {
    filepath="$1"

    if [ -z "$filepath" ]; then
        filepath="./passenger_data.csv"
    elif [ ! -f "$filepath" ]; then
        echo "$filepath doesn't exist"
        return
    fi
    

    less "$filepath"
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

    awk -F'[;,]' '{
        age = $3;

        if (age >= 0 && age <= 18) {
            agegroup = "0-18";
        } else if (age >= 19 && age <= 35) {
            agegroup = "19-35";
        } else if (age >= 36 && age <= 50) {
            agegroup = "36-50";
        } else if (age >= 51) {
            agegroup = "51+";
        }

        ages[agegroup]++;
    } END {

        for (agegroup in ages) {
            print agegroup ":" ages[agegroup] " passengers";
        }

    }' "$input" > ages.txt


awk -F'[;,]' '{
    age = $3;


    if (age >= 0 && age <= 18) {
        agegroup = "0-18";
    } else if (age >= 19 && age <= 35) {
        agegroup = "19-35";
    } else if (age >= 36 && age <= 50) {
        agegroup = "36-50";
    } else if (age >= 51) {
        agegroup = "51+";
    }


    if ( $6 ~ /^[[:space:]]*(Yes|yes)[[:space:]]*$/ ) {
        rescued[agegroup]++;
    }
    total[agegroup]++;
} END {

    for (agegroup in total) {
        if (total[agegroup] > 0) {
            rescpercentage = int((rescued[agegroup] / total[agegroup]) * 100);
            print agegroup ":" rescued[agegroup] "/" total[agegroup] " rescued percentage: "rescpercentage"%";
        } else {
            print agegroup ": No data available for this agegroup";
        }
    }

}' "$input" > percentages.txt


  awk -F'[;,]' '{ 
    age = $3;
    crew_or_pass = $5;

   
    if (age ~ /^[0-9]+([.][0-9]+)?$/) {
        total[crew_or_pass] += age;
        count[crew_or_pass]++;
    }
  } END {
    for (crew_or_pass in total) {
        if (count[crew_or_pass] > 0) {
            average_age = total[crew_or_pass] / count[crew_or_pass];
            print crew_or_pass "s average age is: " average_age;
        }
    }
  }' "$input" > avg.txt


  awk -F'[;,]' '{
   
    if ($6 ~ /^[[:space:]]*(Yes|yes)[[:space:]]*$/) {
        print $0
    }
  }' "$input" > rescued.txt




    echo "Generated:"
    echo "Ageg roups in ages.txt"
    echo " Rescue Percentages in  percentages.txt"
    echo "Average Age per Category in avg.txt"
    echo "Rescued Passengers in rescued.txt"
}




if [ $1 == "insert" ]; then
	insert_data
elif [ $1 == "search" ]; then
	search_passenger
elif [ $1 == "update" ]; then
	update_passenger "$2" "$3" "$4"
elif [ $1 == "display" ]; then
	display_file
elif [ $1 == "reports" ]; then
	generate_reports
else
	echo "Type one of the following functions: insert, search, update, display, reports"
fi
