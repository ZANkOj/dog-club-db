-- ОЧИСТКА

DROP TABLE IF EXISTS disease_history CASCADE;
DROP TABLE IF EXISTS exhibitions CASCADE;
DROP TABLE IF EXISTS parents CASCADE;
DROP TABLE IF EXISTS matings CASCADE;
DROP TABLE IF EXISTS dogs CASCADE;
DROP TABLE IF EXISTS diseases CASCADE;
DROP TABLE IF EXISTS breeds CASCADE;

-- ПОРОДЫ

CREATE TABLE breeds (
    breed_id SERIAL PRIMARY KEY,
    name VARCHAR(100) UNIQUE NOT NULL,
    description TEXT,
    min_height INT CHECK (min_height > 0),
    max_height INT CHECK (max_height >= min_height),
    color VARCHAR(100)
);

-- СОБАКИ

CREATE TABLE dogs (
    dog_id SERIAL PRIMARY KEY,

    breed_id INT NOT NULL
    REFERENCES breeds(breed_id)
    ON DELETE CASCADE,

    nickname VARCHAR(100) NOT NULL,

    gender CHAR(1)
    CHECK (gender IN ('M','F')),

    owner VARCHAR(150),

    address TEXT,

    is_alive BOOLEAN DEFAULT TRUE,

    psyche_score INT
    CHECK (psyche_score BETWEEN 1 AND 5),

    birth_date DATE,

    mating_id INT
);

-- ВЯЗКИ

CREATE TABLE matings (
    mating_id SERIAL PRIMARY KEY,

    male_id INT NOT NULL
    REFERENCES dogs(dog_id),

    female_id INT NOT NULL
    REFERENCES dogs(dog_id),

    mating_date DATE NOT NULL,

    CHECK (male_id <> female_id)
);


-- связь щенков с вязкой
ALTER TABLE dogs
ADD CONSTRAINT fk_mating
FOREIGN KEY (mating_id)
REFERENCES matings(mating_id);

-- РОДИТЕЛИ

CREATE TABLE parents (
    dog_id INT PRIMARY KEY
    REFERENCES dogs(dog_id)
    ON DELETE CASCADE,

    father_id INT
    REFERENCES dogs(dog_id),

    mother_id INT
    REFERENCES dogs(dog_id),

    CHECK (father_id IS NULL OR father_id <> dog_id),
    CHECK (mother_id IS NULL OR mother_id <> dog_id),

    CHECK (
        father_id IS NULL OR
        mother_id IS NULL OR
        father_id <> mother_id
    )
);

-- ВЫСТАВКИ

CREATE TABLE exhibitions (
    exhibition_id SERIAL PRIMARY KEY,

    dog_id INT NOT NULL
    REFERENCES dogs(dog_id)
    ON DELETE CASCADE,

    exhibition_date DATE NOT NULL,

    score INT
    CHECK (score BETWEEN 1 AND 5),

    medal BOOLEAN DEFAULT FALSE
);

-- БОЛЕЗНИ

CREATE TABLE diseases (
    disease_id SERIAL PRIMARY KEY,

    name VARCHAR(100)
    UNIQUE NOT NULL,

    treatment TEXT
);

-- ИСТОРИЯ БОЛЕЗНЕЙ

CREATE TABLE disease_history (
    history_id SERIAL PRIMARY KEY,

    dog_id INT NOT NULL
    REFERENCES dogs(dog_id)
    ON DELETE CASCADE,

    disease_id INT NOT NULL
    REFERENCES diseases(disease_id),

    date_start DATE NOT NULL,

    date_end DATE,

    CHECK (
        date_end IS NULL OR
        date_end >= date_start
    )
);


-- ТРИГГЕР:
-- ПРОВЕРКА ПОЛА РОДИТЕЛЕЙ

CREATE OR REPLACE FUNCTION check_parents_gender()
RETURNS TRIGGER AS $$
DECLARE
    father_gender CHAR(1);
    mother_gender CHAR(1);
BEGIN

    IF NEW.father_id IS NOT NULL THEN

        SELECT gender
        INTO father_gender
        FROM dogs
        WHERE dog_id = NEW.father_id;

        IF father_gender IS DISTINCT FROM 'M' THEN
            RAISE EXCEPTION
            'Father must have gender M';
        END IF;

    END IF;

    IF NEW.mother_id IS NOT NULL THEN

        SELECT gender
        INTO mother_gender
        FROM dogs
        WHERE dog_id = NEW.mother_id;

        IF mother_gender IS DISTINCT FROM 'F' THEN
            RAISE EXCEPTION
            'Mother must have gender F';
        END IF;

    END IF;

    RETURN NEW;
END;
$$ LANGUAGE plpgsql;


CREATE TRIGGER trg_check_parents_gender
BEFORE INSERT OR UPDATE
ON parents
FOR EACH ROW
EXECUTE FUNCTION check_parents_gender();

-- ФУНКЦИЯ:
-- ПРОВЕРКА РОДСТВА

CREATE OR REPLACE FUNCTION check_relation(
    id1 INT,
    id2 INT
)
RETURNS BOOLEAN AS $$
DECLARE
    father1 INT;
    mother1 INT;

    father2 INT;
    mother2 INT;
BEGIN

    SELECT father_id, mother_id
    INTO father1, mother1
    FROM parents
    WHERE dog_id = id1;

    SELECT father_id, mother_id
    INTO father2, mother2
    FROM parents
    WHERE dog_id = id2;

    IF id1 = father2 OR
       id1 = mother2 OR
       id2 = father1 OR
       id2 = mother1
    THEN
        RETURN TRUE;
    END IF;

    IF (
        father1 IS NOT NULL AND
        father1 = father2
    )
    OR
    (
        mother1 IS NOT NULL AND
        mother1 = mother2
    )
    THEN
        RETURN TRUE;
    END IF;

    RETURN FALSE;
END;
$$ LANGUAGE plpgsql;

-- ИНДЕКСЫ

CREATE INDEX idx_exhibitions
ON exhibitions(dog_id);

CREATE INDEX idx_disease_history
ON disease_history(dog_id);

CREATE INDEX idx_parents
ON parents(father_id, mother_id);

-- ТРИГГЕР:
-- ПРОВЕРКА ВЯЗКИ

CREATE OR REPLACE FUNCTION check_mating_gender()
RETURNS TRIGGER AS $$
DECLARE
    male_gender CHAR(1);
    female_gender CHAR(1);
BEGIN

    SELECT gender
    INTO male_gender
    FROM dogs
    WHERE dog_id = NEW.male_id;

    IF male_gender IS DISTINCT FROM 'M' THEN
        RAISE EXCEPTION
        'Male dog must have gender M';
    END IF;

    SELECT gender
    INTO female_gender
    FROM dogs
    WHERE dog_id = NEW.female_id;

    IF female_gender IS DISTINCT FROM 'F' THEN
        RAISE EXCEPTION
        'Female dog must have gender F';
    END IF;

    RETURN NEW;
END;
$$ LANGUAGE plpgsql;


CREATE TRIGGER trg_check_mating_gender
BEFORE INSERT OR UPDATE
ON matings
FOR EACH ROW
EXECUTE FUNCTION check_mating_gender();

-- ПРОСМОТР ТАБЛИЦ
SELECT table_name
FROM information_schema.tables
WHERE table_schema = 'public';


SELECT * FROM breeds;
SELECT * FROM dogs;
SELECT * FROM parents;
SELECT * FROM matings;
SELECT * FROM exhibitions;
SELECT * FROM diseases;
SELECT * FROM disease_history;
